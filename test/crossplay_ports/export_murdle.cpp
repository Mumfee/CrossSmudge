#include "MurdleCore.h"
#include "MurdleCast.h"
#include "MurdleText.h"
#include <cstdio>
#include <cassert>
#include <string>
static void quote(const char*s){putchar('"');for(;*s;s++){if(*s=='"'||*s=='\\')putchar('\\');putchar(*s);}putchar('"');}
int main(int argc,char**argv){int tier=std::stoi(argv[1]),id=std::stoi(argv[2]);uint32_t seed=0x1234567u+id*7919u+tier*1000003u;
 murdle::Scratch scratch{};murdle::Puzzle p{};auto t=static_cast<murdle::Tier>(tier);auto shape=murdle::shapeOf(t);uint8_t cast[4][4]{};
 assert(murdle::drawCast(seed,shape,cast));assert(murdle::generate(t,seed,cast,murdle::attrMasksFor(cast,shape),scratch,p));assert(murdle::countSolutions(p,2,scratch)==1);murdle::Grid grid;assert(murdle::deduce(p,grid)>=0);
 printf("return {seed=%u,n=%d,cats=%d,murder=%d,solution={",seed,p.shape.items,p.shape.cats,p.murderRow+1);
 for(int c=0;c<p.shape.cats;c++){printf("{");for(int r=0;r<p.shape.items;r++)printf("%d,",p.assign[c][r]+1);printf("},");}printf("},labels={");
 for(int c=0;c<p.shape.cats;c++){printf("{");for(int i=0;i<p.shape.items;i++){quote(murdletext::label(p,c,i));printf(",");}printf("},");}printf("},dossier={");
 char line[200];for(int i=0;i<p.shape.items;i++){murdletext::suspectAttributes(p,i,line,sizeof(line));quote((std::string(murdletext::label(p,0,i))+": "+line).c_str());printf(",");}
 for(int c=1;c<p.shape.cats;c++)for(int i=0;i<p.shape.items;i++){auto tr=murdletext::trait(p,c,i);quote((std::string(murdletext::label(p,c,i))+": "+tr).c_str());printf(",");}
 printf("},clues={");for(int i=0;i<p.clueCount;i++){auto&c=p.clues[i];printf("{%d,%d,%d,%d,%d,%d},",int(c.anchor),c.anchorCat+1,c.anchorItem+1,c.targetCat+1,c.targetMask,c.speaker==255?0:c.speaker+1);}printf("},text={");
 for(int i=0;i<p.clueCount;i++){murdletext::clueLine(p,i,line,sizeof(line));quote(line);printf(",");}printf("}}\n");}
