#include "JaipurCore.h"
#include "SeaSaltCore.h"
#include "ToyBattleCore.h"
#include "GoCore.h"
#include <iostream>
#include <string>
#include <cstdint>
// Line protocol, one source-core match at a time. 1-based Lua-facing indices.
int main(int argc,char**argv){std::string type=argv[1];uint32_t seed=std::stoul(argv[2]);
 if(type=="jaipur"){jaipur::Game g;g.newGame(seed,0);int k;while(std::cin>>k){if(k==9){uint32_t s;std::cin>>s;g.startNextRound(s);}else{jaipur::Move m;m.kind=static_cast<jaipur::Move::Kind>(k==4?3:k==3?2:k-1);int x,n;std::cin>>x>>n;m.slot=x-1;m.good=x-1;m.count=n;if(k==3){m.marketMask=x;for(int i=0;i<6;i++){std::cin>>n;m.give[i]=n;}std::cin>>n;m.giveCamels=n;}std::cout<<g.apply(m)<<' ';}
 std::cout<<int(g.phase)<<' '<<int(g.turn)+1<<' '<<int(g.deckTaken)+1<<' '<<g.score(0)<<' '<<g.score(1)<<' '<<int(g.seals[0])<<' '<<int(g.seals[1]);for(auto c:g.market)std::cout<<' '<<int(c)+1;for(auto&h:g.hand)for(auto c:h)std::cout<<' '<<int(c);std::cout<<' '<<int(g.herd[0])<<' '<<int(g.herd[1])<<'\n';}}
 else if(type=="seasalt"){seasalt::Game g;g.newGame(seed,0);int k,x,y;while(std::cin>>k>>x>>y){bool ok=false;switch(k){case 1:ok=g.takeFromDeck();break;case 2:ok=g.takeFromPile(x-1);break;case 3:ok=g.keepDrawn(x-1);break;case 4:ok=g.discardTo(x-1);break;case 5:ok=g.playDuo(x-1,y-1);break;case 6:ok=g.chooseCrabPile(x-1);break;case 7:ok=g.takeCrabCard(x-1);break;case 8:ok=g.endTurn();break;case 9:ok=g.endRound(x);break;case 10:g.deal(uint32_t(x),g.ender!=255?1-g.ender:1-g.roundStarter);g.round++;ok=true;break;}
 std::cout<<ok<<' '<<int(g.phase)<<' '<<int(g.step)<<' '<<int(g.turn)+1<<' '<<int(g.deckNext)+1<<' '<<g.cardPoints(0)<<' '<<g.cardPoints(1)<<' '<<int(g.score[0])<<' '<<int(g.score[1]);for(auto c:g.place)std::cout<<' '<<int(c);std::cout<<'\n';}}
 else if(type=="go"){go::Game g;go::reset(g,argc>3?atoi(argv[3]):9);int p;while(std::cin>>p){bool ok=go::play(g,p==0?go::kPass:p-1);auto s=go::score(g);std::cout<<ok<<' '<<int(g.stage)<<' '<<int(g.toMove)<<' '<<g.moveNumber<<' '<<s.blackHalves<<' '<<s.whiteHalves;for(int i=0;i<g.points();i++)std::cout<<' '<<int(g.at(i));std::cout<<'\n';}}
 else {toybattle::Game g;g.newGame(seed,argc>3?atoi(argv[3]):0,0);int n;while(std::cin>>n){toybattle::Move m;if(n){m.kind=toybattle::Move::Kind::Place;m.stepCount=n;for(int i=0;i<n;i++){int v[8];for(auto&x:v)std::cin>>x;auto&s=m.steps[i];s.slot=v[0]-1;s.kind=v[1]-1;s.useEffect=v[2];s.target=v[3]?v[3]-1:255;s.useBase=v[4];s.baseFrom=v[5]?v[5]-1:255;s.baseTo=v[6]?v[6]-1:255;s.baseKind=v[7]-1;}}
 std::cout<<g.apply(m)<<' '<<int(g.phase)<<' '<<int(g.turn)+1<<' '<<(g.winner==255?0:int(g.winner)+1)<<' '<<int(g.ending)<<' '<<int(g.medals[0])<<' '<<int(g.medals[1]);for(auto&h:g.rack)for(auto c:h)std::cout<<' '<<int(c);std::cout<<' '<<int(g.placementCount);for(int i=0;i<g.placementCount;i++)std::cout<<' '<<int(g.placeSlot[i])+1<<' '<<int(g.placeTile[i]);std::cout<<'\n';}}
}
