local N=dofile('core.lua');local S=dofile('storage.lua')
local doc,lines,index,selected,screen,choice='',{},1,1,'library',1
local offset=0;local notice='';local w,h=480,800;local margin=25;local pageRows=8;local pending=''
local function save()
 collectgarbage('collect')
 if not S.save(doc) then notice='Save failed. Keep app open and retry.';return false end
 return true
end
local function load(index_)
 index=index_;selected=1;offset=0;doc='';lines={};collectgarbage('collect');S.scope(index)
 local restored=S.load(N.validate)
 if restored then doc=restored else
  local raw=smudge.read_file('note'..index..'.md',3073)
  doc=raw and N.validate(raw) or ''
  if raw and not N.validate(raw) then notice='Import rejected: over 3072 bytes/97 lines or NUL.' end
 end
 lines=N.parse(doc,true);screen='note'
end
local function redraw()smudge.request_update()end
local options={'Resume','Clear checked lines','Convert to checklist','Convert to page','Import from note file','Help','Save and close'}
local function change(new)
 S.last=nil;offset=0;doc=new;lines=nil;collectgarbage('collect');lines=N.parse(doc,true);selected=math.min(selected,#lines);redraw()
end
local function operate()
 if choice==1 then screen='note'
 elseif choice==2 or choice==5 then pending=choice==2 and 'clear' or 'import';screen='confirm';choice=1
 elseif choice==3 then change(N.tolist(doc,lines));screen='note'
 elseif choice==4 then change(N.strip(doc,lines));screen='note'
 elseif choice==6 then screen='help'
 elseif save() then screen='library';choice=index end
 redraw()
end
function on_init()w,h=smudge.get_bounds();redraw()end
function on_draw()
 w,h=smudge.get_bounds();margin=w>h and 64 or 25;pageRows=math.max(1,(h-180)//36)
 smudge.clear();smudge.header('CP Notes',screen=='library' and 'Eight app-local cards' or 'note'..index..'.md')
 smudge.rect(w-105,60,85,30,false);smudge.text(w-98,65,'Menu','small')
 if screen=='library' then
  for i=1,8 do smudge.text(margin,95+(i-1)*36,(choice==i and '> ' or '  ')..'Note '..i,'ui12') end
 elseif screen=='note' then
  local start=((selected-1)//pageRows)*pageRows+1
  for i=start,math.min(#lines,start+pageRows-1) do
   local l=N.row(lines[i]);local text=doc:sub(l.text,l.last)
   local prefix=(selected==i and '> ' or '  ')..(l.mark and (l.checked and '[x] ' or '[ ] ') or '')
   -- Horizontal scrolling is explicit with Left/Right so long prose is retained.
   local max=math.max(1,(w-margin-35)//10)
   local scroll=selected==i and offset or 0
   smudge.text(margin,95+(i-start)*36,prefix..N.slice(text,scroll+1,scroll+max),'small')
  end
  if doc=='' then smudge.text(margin,95,'Empty. Copy note'..index..'.md here, then Import.','small') end
 elseif screen=='menu' then
  for i,label in ipairs(options) do smudge.text(margin,95+(i-1)*36,(choice==i and '> ' or '  ')..label,'small') end
 elseif screen=='confirm' then
  smudge.text(margin,95,pending=='clear' and 'Remove all checked lines?' or 'Replace edits with note file?','small')
  smudge.text(margin,150,(choice==1 and '> ' or '  ')..'Cancel','ui12')
  smudge.text(margin,198,(choice==2 and '> ' or '  ')..'Confirm','ui12')
 elseif screen=='help' then
  local help={'Copy note1.md ... note8.md into this app folder.',
   'Each card: at most 3072 bytes and 97 lines.', 'Use Markdown checkboxes: - [ ] task or - [x] task.',
   'Confirm toggles one checkbox. Left/right scroll text.', 'Back opens the card menu; imports need confirmation.',
   'Original .md imports stay unchanged; journals hold edits.', 'Phone transfer and sleep-screen notes need host support.'}
  local y=95
  for _,line in ipairs(help) do
   local current=''
   for word in line:gmatch('%S+')do
    local next_line=current=='' and word or current..' '..word
    if current~='' and smudge.text_width(next_line,'small')>w-margin-20 then
     smudge.text(margin,y,current,'small');y=y+25;current=word
    else current=next_line end
   end
   smudge.text(margin,y,current,'small');y=y+30
  end
 end
 if notice~='' then smudge.text(margin,h-85,notice,'small') end
 smudge.button_hints('Menu','Select','Up','Down')
end
function on_button(btn,pressed)
 if not pressed then return end
 if btn=='page_back' then btn='up' elseif btn=='page_forward' then btn='down' end
 if btn=='back' then
  if screen=='library' then smudge.exit()
  elseif screen=='note' then screen='menu';choice=1
  elseif screen=='menu' then if save() then screen='library';choice=index end
  else screen='menu';choice=1 end
 elseif screen=='library' then
  if btn=='up' then choice=(choice-2)%8+1 elseif btn=='down' then choice=choice%8+1
  elseif btn=='confirm' then notice='';load(choice) end
 elseif screen=='note' then
  if btn=='up' then selected=(selected-2)%#lines+1;offset=0 elseif btn=='down' then selected=selected%#lines+1;offset=0
  elseif btn=='confirm' then
   local line=N.row(lines[selected])
   if line.mark then S.last=nil;collectgarbage('collect');doc=N.toggle(doc,selected,lines);line.checked=not line.checked;lines[selected]=N.pack(line) end
  elseif btn=='right' or btn=='left' then
   local l=N.row(lines[selected]);offset=math.max(0,math.min(math.max(0,l.last-l.text), offset+(btn=='right' and 20 or -20)))
  end
 elseif screen=='menu' then
  if btn=='up' then choice=(choice-2)%#options+1 elseif btn=='down' then choice=choice%#options+1
  elseif btn=='confirm' then operate() end
 elseif screen=='confirm' then
  if btn=='up' or btn=='down' then choice=3-choice
  elseif btn=='confirm' then
   if choice==2 then
    if pending=='clear' then change(N.clear(doc,lines)) else
     local raw=smudge.read_file('note'..index..'.md',3073)
     if N.validate(raw) then change(raw);notice='' else notice='Import failed: missing, invalid, or too large.' end
    end
   end
   screen='note'
  end
 end
 redraw()
end
function on_touch(x,y,event)
 if event~='tap' or x<0 or y<0 or x>=w or y>=h then return end
 if x>=w-105 and x<w-20 and y>=60 and y<90 then on_button('back',true);return end
 local row=(y-95)//36+1
 if screen=='library' and row>=1 and row<=8 then choice=row;on_button('confirm',true)
 elseif screen=='menu' and row>=1 and row<=#options then choice=row;operate()
 elseif screen=='note' and row>=1 and row<=pageRows then
  local i=((selected-1)//pageRows)*pageRows+row
  if i<=#lines then selected=i;offset=0;on_button('confirm',true) end
 elseif screen=='confirm' and y>=145 and y<240 then choice=y<193 and 1 or 2;on_button('confirm',true)
 elseif screen=='help' then on_button('back',true) end
end
function on_swipe(dir)on_button(dir,true)end
function on_update()end
function on_exit()if screen~='library' then save() end end
