// Independent oracle: compile original freestanding CrossPlay C++ rules/brains.
// Usage: differential <minesweeper|knucklebones|connectfour>; stdout is a Lua fixture.
#include <cstdint>
#include <initializer_list>
#include <cstdio>
#include <cstring>
#include "minesweeper/MinesweeperCore.h"
#include "knucklebones/KnucklebonesCore.h"
#include "knucklebones/KnucklebonesBrain.h"
#include "connectfour/ConnectFourCore.h"
#include "connectfour/ConnectFourBrain.h"
static void minesState(const minesweeper::Game& g) {
 printf("'%d|%d|",static_cast<int32_t>(g.rng),static_cast<int>(g.status));
 for(int y=0;y<10;y++)for(int x=0;x<8;x++)printf("%d",g.cell[x][y]);
 printf("'");
}
static void knuckleState(const knucklebones::Game& g) {
 int status=knucklebones::over(g)?(knucklebones::winner(g)<0?3:knucklebones::winner(g)+1):0;
 printf("'%d|%d%d%d|",static_cast<int32_t>(g.rng),g.turn+1,g.die,status);
 for(int s=0;s<2;s++)for(int x=0;x<3;x++)for(int y=0;y<3;y++)printf("%d",g.grid[s].cell[x][y]);
 printf("'");
}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 puts("local M=dofile('core.lua'); assert(math.maxinteger==2147483647,'must use stock int32 Lua'); local g");
 if(!strcmp(argv[1],"minesweeper")) {
  for(uint32_t seed: {0u,1u,2u,7u,25u,100u,123456u,0xffffffffu}) {
   minesweeper::Game g;minesweeper::start(g,seed);
   printf("g=M.new(%d)\n",static_cast<int32_t>(seed));
   // One first dig plus many deterministic digs/flags, checked against source byte-for-byte.
   for(int j=0;j<10;j++) {
    int x=(j*7+3)%8,y=(j*3+4)%10;bool flag=j%4==1;
    bool ok=flag?minesweeper::toggleFlag(g,x,y):minesweeper::dig(g,x,y);
    printf("assert(M.action(g,%d,%d,%s)==%s);assert(M.encode(g)==",x+1,y+1,flag?"true":"false",ok?"true":"false");
    minesState(g);puts(")");
   }
  }
  // Construct wrong-but-count-matching flags around a revealed number.
  bool found=false;
  for(uint32_t seed=1;seed<100 && !found;seed++) {
   minesweeper::Game g;minesweeper::start(g,seed);minesweeper::dig(g,3,4);
   for(int x=0;x<8 && !found;x++)for(int y=0;y<10 && !found;y++) {
    if(!minesweeper::has(g,x,y,minesweeper::kRevealed))continue;
    int needed=minesweeper::neighbouringMines(g,x,y);if(!needed)continue;
    int safeX=-1,safeY=-1;
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)
     if(minesweeper::inside(x+dx,y+dy)&&g.cell[x+dx][y+dy]==0){safeX=x+dx;safeY=y+dy;}
    if(safeX<0)continue;
    auto trial=g;minesweeper::toggleFlag(trial,safeX,safeY);int remain=needed-1;
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)
     if(remain>0 && minesweeper::has(trial,x+dx,y+dy,minesweeper::kMine)) {
      minesweeper::toggleFlag(trial,x+dx,y+dy);remain--;
     }
    printf("g=assert(M.decode(");minesState(trial);puts("))");
    minesweeper::dig(trial,x,y);
    printf("assert(M.action(g,%d,%d,false));assert(g.status==3);assert(M.encode(g)==",x+1,y+1);
    minesState(trial);puts(",'wrong flag chord matches original')");found=true;
   }
  }
 } else if(!strcmp(argv[1],"knucklebones")) {
  for(uint32_t seed: {0u,1u,2u,7u,25u,100u,123456u,0xffffffffu}) {
   knucklebones::Game g;knucklebones::start(g,seed);
   printf("g=M.new(%d);assert(M.encode(g)==",static_cast<int32_t>(seed));knuckleState(g);puts(")");
   for(int j=0;j<60 && !knucklebones::over(g);j++) {
    int c=knucklebones::chooseColumn(g);
    printf("assert(M.choose(g)==%d,'brain choice');assert(M.action(g,%d));assert(M.encode(g)==",c+1,c+1);
    knucklebones::place(g,c);knuckleState(g);puts(")");
   }
  }
 } else if(!strcmp(argv[1],"connectfour")) {
  // These positions include empty/early and developed boards and immediate tactical wins.
  const char* positions[]={"","123","12121","121213","172737","12233434445","4455433226","1234567","72726561","1727374","12234334544","76654554344"};
  for(auto p:positions){connectfour::Game g;connectfour::start(g);for(const char*q=p;*q;q++)connectfour::drop(g,*q-'1');
   int c=connectfour::chooseColumn(g);
   printf("g=assert(M.decode('%s'));assert(M.choose(g)==%s",p,c<0?"nil":"");if(c>=0)printf("%d",c+1);puts(",'seven-ply source AI')");
  }
  // Full deterministic legal games cover board representation, terminal outcomes, gravity.
  for(uint32_t seed=1;seed<=10;seed++) {
   uint32_t rng=seed;connectfour::Game g;connectfour::start(g);puts("g=M.new()");
   while(!connectfour::over(g)) {
    rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;int c=rng%7;
    while(!connectfour::canDrop(g,c))c=(c+1)%7;
    connectfour::drop(g,c);
    printf("assert(M.action(g,%d));assert(g.status==%d);assert(g.turn==%d);assert(table.concat(g.b)=='",c+1,static_cast<int>(g.outcome),g.turn);
    for(int y=0;y<6;y++)for(int x=0;x<7;x++)printf("%d",g.cell[x][y]);puts("')");
   }
  }
 } else return 2;
 puts("print('C++ differential oracle OK')");
}
