// Original CalcEngine + original IBM decNumber oracle, no source modifications.
#include "CalcEngine.h"
#include <iostream>
#include <string>
static calc::Key key(char c){
 if(c>='0'&&c<='9')return static_cast<calc::Key>(static_cast<int>(calc::Key::D0)+c-'0');
 switch(c){case '.':return calc::Key::Dot;case '+':return calc::Key::Add;case '-':return calc::Key::Sub;case 'x':return calc::Key::Mul;case '/':return calc::Key::Div;case '=':return calc::Key::Equals;case '%':return calc::Key::Percent;case 'n':return calc::Key::Negate;case 'C':return calc::Key::ClearAll;case 'E':return calc::Key::ClearEntry;case '<':return calc::Key::Backspace;default:return calc::Key::None;}
}
int main(){std::string s;while(std::getline(std::cin,s)){calc::Engine e;bool first=true;for(char c:s){if(c==' ')continue;e.press(key(c));if(!first)std::cout<<'|';std::cout<<e.display();first=false;}std::cout<<'\n';}}
