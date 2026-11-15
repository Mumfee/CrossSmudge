#include <iostream>
#include <string>
#include "/root/github/crossplay/src/apps_local/checkers/CheckersBrain.h"
using namespace checkers;
static unsigned rng=424242;
static unsigned next(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static std::string moveText(const Move&m){std::string s=std::to_string(m.from+1)+":"+std::to_string(m.to+1);for(int i=0;i<64;i++)if(tookSquare(m,i))s+=":"+std::to_string(i+1);return s;}
int main(){int samples=0;for(int game=0;game<100;game++){Game g{};start(g);for(int ply=0;ply<500&&!over(g);ply++){
 Move moveset[48];int n=moves(g,moveset);bool deep=false;for(int i=0;i<n;i++)deep|=takenCountOf(moveset[i])>=4;
 if((samples<150&&ply%13==0)||deep){samples++;std::cout<<int(g.turn+1)<<","<<int(g.idlePlies)<<",";for(int i=0;i<64;i++){auto p=g.cell[i];std::cout<<(p==0?0:(isDark(g,i)?2:1)+(isKing(g,i)?2:0));}std::cout<<";";for(int i=0;i<n;i++)std::cout<<(i?"/":"")<<moveText(moveset[i]);Move best{};chooseMove(g,best);std::cout<<";"<<moveText(best)<<"\n";}
 play(g,moveset[next()%n]);
 }}}
