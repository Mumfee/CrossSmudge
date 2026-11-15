#include "ToyBattleCore.h"
#include <cstdio>
#include <cstdlib>
int main(int argc,char**argv){auto&t=toybattle::terrainAt(atoi(argv[1]));printf("return {name='%s',n=%d,hq={",t.name,t.baseCount);
 for(int i=0;i<t.hqCount;i++)printf("%d,",t.hqSeat[i]+1);printf("},goal=%d,edges={",t.medalsObjective);
 for(int i=0;i<t.edgeCount;i++)printf("{%d,%d},",t.edges[i].a+1,t.edges[i].b+1);printf("},regions={");
 for(int i=0;i<t.regionCount;i++){printf("{%u,%d},",t.regions[i].bases,t.regions[i].medals);}printf("},special={");
 for(int i=0;i<t.baseCount;i++)printf("%d,",t.special[i]);printf("},gate={");for(int i=0;i<t.slotCount();i++)printf("%d,",t.gate[i]);puts("}}");}
