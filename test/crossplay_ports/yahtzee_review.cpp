// Original C++ oracle for Yahtzee rules, Joker and exact expected-value brain.
#include "yahtzee/YahtzeeCore.h"
#include "yahtzee/YahtzeeBrain.h"
#include <cstdio>
#include <cstdint>
#include <initializer_list>
static void state(const yahtzee::Game&g){printf("'%d,%d,%d,%d",static_cast<int32_t>(g.rng),g.turn+1,g.rollsUsed,g.held);for(int i=0;i<5;i++)printf(",%d",g.die[i]);for(int p=0;p<2;p++){for(int i=0;i<13;i++)printf(",%d",g.card[p].box[i]);printf(",%d",g.card[p].yahtzeeBonuses);}putchar('\'');}
static void check(const yahtzee::Game&g){printf("assert(Y.encode(g)==");state(g);puts(",'source state')");}
int main(){
 puts("local Y=dofile('core.lua');local g,h,c");
 uint8_t dice[5];yahtzee::forEachOutcome(dice,5,0,1,[&](const uint8_t*d){
  printf("h={%d,%d,%d,%d,%d}\n",d[0],d[1],d[2],d[3],d[4]);
  for(int i=0;i<13;i++)printf("assert(Y.raw(h,%d)==%d,'raw box')\n",i+1,yahtzee::rawScore(d,static_cast<yahtzee::Category>(i)));
 });
 for(int face=1;face<=6;face++)for(int ybox:{-1,0,50})for(int upper:{-1,0})for(int lower:{-1,0}){
  yahtzee::Card card;yahtzee::startCard(card);card.box[11]=ybox;card.box[face-1]=upper;
  if(lower==0)for(int i=6;i<13;i++)if(i!=11)card.box[i]=0;
  printf("h={%d,%d,%d,%d,%d};c={",face,face,face,face,face);
  for(int i=0;i<13;i++)printf("%s%d",i?",":"",card.box[i]);puts(",0}");
  for(int i=0;i<5;i++)dice[i]=face;
  for(int i=0;i<13;i++)printf("assert(Y.allowed(c,h,%d)==%s);assert(Y.score_for(c,h,%d)==%d)\n",i+1,yahtzee::canScore(card,dice,static_cast<yahtzee::Category>(i))?"true":"false",i+1,yahtzee::scoreFor(card,dice,static_cast<yahtzee::Category>(i)));
 }
 for(uint32_t seed:{0u,123456u,0xffffffffu}){
  yahtzee::Game game;yahtzee::start(game,seed);
  while(!yahtzee::over(game)){
   printf("g=assert(Y.decode(");state(game);puts("))");
   for(int roll=0;roll<3;roll++){
    if(roll>0){int hold=yahtzee::chooseHold(game.card[game.turn],game.die);
     printf("do local s=Y.begin(g);local done,m;for i=1,32 do done,m=Y.step(g,s) end;assert(done and m==%d,'source hold');g.held=m end\n",hold);game.held=hold;
    }
    yahtzee::roll(game);puts("assert(Y.roll(g))");check(game);
   }
   auto box=yahtzee::chooseBox(game.card[game.turn],game.die);printf("assert(Y.choose_box(g.cards[g.turn],g.d)==%d,'source box');assert(Y.take(g,%d))\n",static_cast<int>(box)+1,static_cast<int>(box)+1);yahtzee::take(game,box);check(game);
   printf("assert(Y.total(g.cards[1])==%d and Y.total(g.cards[2])==%d)\n",yahtzee::total(game.card[0]),yahtzee::total(game.card[1]));
  }
 }
}
