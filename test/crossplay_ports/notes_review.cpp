// Independent NotesCore oracle. CrossPlay source remains unmodified.
#include "notes/NotesCore.h"
#include <cstdio>
#include <string>
#include <vector>
static void quote(const std::string& s){putchar('"');for(unsigned char c:s)printf("\\%03u",c);putchar('"');}
int main(){
 puts("local N=dofile('core.lua');local s,p");
 const std::vector<std::string> docs={"","milk","- [ ] milk\n- [x] bread\n","  * [X] indented\r\nprose\r\n","+ [ ]","- [] nope\n- [x]tra credit\n- [ ] okay", "one\n\n- [X] two\nthree", "\t+ [ ] a\n \t* [x] b", " - [x] \r\n","- [ ] café\n","standalone\r"};
 for(const auto& doc:docs){
  printf("s=");quote(doc);puts(";p=N.parse(s)");auto lines=notes::parse(doc);
  printf("assert(#p==%zu,'line count')\n",lines.size());
  for(size_t i=0;i<lines.size();i++){
   const auto &l=lines[i];printf("assert(p[%zu].first==%zu and p[%zu].last==%zu and p[%zu].text==%zu,'ranges')\n",i+1,l.begin+1,i+1,l.end,i+1,l.textBegin+1);
   printf("assert((p[%zu].mark~=nil)==%s,'task')\n",i+1,l.isTask?"true":"false");
   if(l.isTask){printf("assert(p[%zu].mark==%zu and p[%zu].checked==%s)\n",i+1,l.markAt+1,i+1,l.checked?"true":"false");}
   auto changed=doc;auto copy=l;notes::toggle(changed,copy);printf("assert(N.toggle(s,%zu)==",i+1);quote(changed);puts(",'toggle')");
  }
  auto changed=doc;notes::clearChecked(changed);printf("assert(N.clear(s)==");quote(changed);puts(",'clear')");
  changed=doc;notes::stripMarkers(changed);printf("assert(N.strip(s)==");quote(changed);puts(",'strip')");
  changed=doc;notes::coerceToList(changed);printf("assert(N.tolist(s)==");quote(changed);puts(",'tolist')");
 }
 puts("print('Notes original C++ core differential OK')");
}
