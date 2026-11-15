local U=dofile('util.lua');local M={title='Murdle - 64 local cases'}
local cached,case
function M.case(g)if cached~=g.id then case=dofile('data/'..g.id..'.lua');cached=g.id end;return case end
function M.new(seed,id)
 id=id or U.mod(seed,64)+1
 local g={id=id,seed=seed,phase=0,ui=0,block=1,cell=1,murder=1,picks={1,1,1,1},attempts=0,marks={},ticks=0}
 local p=M.case(g);g.marks=U.zeros(p.cats*(p.cats-1)//2*p.n*p.n);return g
end
function M.blocks(p)local a={};for c=1,p.cats-1 do for d=c+1,p.cats do a[#a+1]={c,d} end end;return a end
function M.shown(g,b,r,c)
 local p=M.case(g);local n=p.n;local i=(b-1)*n*n+(r-1)*n+c;local v=g.marks[i];if v~=0 then return v end
 for j=1,n do if g.marks[(b-1)*n*n+(r-1)*n+j]==2 or g.marks[(b-1)*n*n+(j-1)*n+c]==2 then return 1 end end;return 0
end
function M.tap(g,b,r,c)
 if g.phase~=0 then return false end
 local p=M.case(g);local n=p.n;local v=(M.shown(g,b,r,c)+1)%3
 if v==2 then for j=1,n do if (j~=c and g.marks[(b-1)*n*n+(r-1)*n+j]==2) or (j~=r and g.marks[(b-1)*n*n+(j-1)*n+c]==2) then return false end end end
 g.marks[(b-1)*n*n+(r-1)*n+c]=v;return true
end
function M.holds(clue,assign,murder)
 local row=murder
 if clue[1]==0 then for r,v in ipairs(assign[clue[2]]) do if v==clue[3] then row=r;break end end end
 local yes=(clue[5]&(1<<(assign[clue[4]][row]-1)))~=0
 return clue[6]==0 and yes or clue[6]~=0 and (yes==(clue[6]~=murder))
end
function M.accuse(g)
 if g.phase~=0 then return false end
 local p=M.case(g);g.attempts=g.attempts+1
 if g.picks[1]~=p.murder then return false end
 for c=2,p.cats do if g.picks[c]~=p.solution[c][p.murder] then return false end end
 g.phase=1;return true
end
function M.valid(g)
 if g.id<1 or g.id>64 or g.phase<0 or g.phase>1 or g.attempts<0 or g.attempts>100000 then return false end
 local p=M.case(g);if not U.array(g.picks,4,1,p.n) then return false end;local blocks=p.cats*(p.cats-1)//2
 if g.murder<1 or g.murder>p.n or g.block<1 or g.block>blocks or not U.array(g.marks,blocks*p.n*p.n,0,2) then return false end
 for b=1,blocks do for r=1,p.n do local row,col=0,0;for c=1,p.n do if g.marks[(b-1)*p.n*p.n+(r-1)*p.n+c]==2 then row=row+1 end;if g.marks[(b-1)*p.n*p.n+(c-1)*p.n+r]==2 then col=col+1 end end;if row>1 or col>1 then return false end end end
 return true
end
U.codec(M,{'id','seed','phase','ui','block','cell','murder','picks','attempts','marks','ticks'})
function M.pause(g)g.ui=0 end
local function ui()dofile('ui.lua')(M,U) end
function M.view(g)ui();return M.view(g) end
function M.act(g,i)ui();return M.act(g,i) end

return M
