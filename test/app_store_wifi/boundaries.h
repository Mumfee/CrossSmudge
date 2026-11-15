#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

struct Rect { int x=0,y=0,width=0,height=0; };
struct ThemeMetrics { int topPadding=10,headerHeight=45,contentSidePadding=20,buttonHintsHeight=0; };
namespace EpdFontFamily { enum Style { REGULAR, BOLD }; }
constexpr int UI_12_FONT_ID=12, UI_10_FONT_ID=10, SMALL_FONT_ID=8;
constexpr int STR_BACK=1, STR_CONNECT_WIFI=2;
const char* tr(int id) { return id==STR_BACK ? "Back" : "Connect Wi-Fi"; }
bool wifiConnected=false;
int fetches=0;
struct GfxRenderer {
  int w=480,h=800;
  std::array<int,4> safe{17,19,23,29};
  mutable std::vector<Rect> outlines;
  mutable std::vector<std::string> texts;
  int getScreenWidth() const { return w; }
  int getScreenHeight() const { return h; }
  void getOrientedViewableTRBL(int* t,int* r,int* b,int* l) const {*t=safe[0];*r=safe[1];*b=safe[2];*l=safe[3];}
  void clearScreen() { outlines.clear(); texts.clear(); }
  void displayBuffer() {}
  template<class... T> void drawCenteredText(int,int,const char* s,T...) const { texts.emplace_back(s); }
  void drawRect(int x,int y,int w,int h,bool) const { outlines.push_back({x,y,w,h}); }
  void fillRect(int,int,int,int,bool) const {}
  int getTextWidth(int,const char*,EpdFontFamily::Style) const { return 90; }
  int getLineHeight(int) const { return 20; }
  void drawText(int,int,int,const char* s,bool,EpdFontFamily::Style) const { texts.emplace_back(s); }
};
struct MappedInputManager {
  enum class Button { Back,Confirm };
  bool touch=true,tapped=false,confirm=false,back=false;
  int x=0,y=0;
  bool hasTouch() const { return touch; }
  bool hasTouchHardware() const { return touch; }
  bool wasReleased(Button b) const { return b==Button::Back ? back : confirm; }
  bool wasScreenTapped(int& tx,int& ty) const { tx=x; ty=y; return touch&&tapped; }
  bool wasTapInRect(int rx,int ry,int rw,int rh) const { return touch&&tapped&&x>=rx&&x<rx+rw&&y>=ry&&y<ry+rh; }
  struct Labels {const char *btn1,*btn2,*btn3,*btn4;};
  Labels mapLabels(const char* a,const char* b,const char* c,const char* d) {return {a,b,c,d};}
  void tap(int tx,int ty) {reset(); tapped=true;x=tx;y=ty;}
  void reset() {tapped=confirm=back=false;}
};
struct UITheme {
  ThemeMetrics metrics;
  static UITheme& getInstance() {static UITheme theme;return theme;}
  const ThemeMetrics& getMetrics() const {return metrics;}
  template<class... T> void drawHeader(T&&...) {}
  template<class... T> void drawButtonHints(T&&...) {}
};
#define GUI UITheme::getInstance()
#define LOG_ERR(...) ((void)0)
struct RenderLock {};
struct ActivityResult {};
struct Activity {
  GfxRenderer& renderer;
  MappedInputManager& mappedInput;
  std::function<void(const ActivityResult&)> callback;
  int children=0,updates=0,waits=0;
  bool finished=false;
  Activity(const char*,GfxRenderer& r,MappedInputManager& i):renderer(r),mappedInput(i){}
  virtual ~Activity()=default;
  virtual void onEnter(){}
  virtual void onExit(){}
  virtual void loop(){}
  virtual void render(RenderLock&&){}
  void requestUpdate(){++updates;}
  void requestUpdateAndWait(){++waits;}
  void finish(){finished=true;}
  template<class T,class F> void startActivityForResult(std::unique_ptr<T> child,F cb){if(child){++children;callback=cb;}}
};
struct WifiSelectionActivity {WifiSelectionActivity(GfxRenderer&,MappedInputManager&) {}};
bool failAllocation = false;
template<class T,class... A> std::unique_ptr<T> makeUniqueNoThrow(A&&... a){if(failAllocation) return nullptr;return std::make_unique<T>(std::forward<A>(a)...);}
struct FontSystem {void releaseForNetwork(GfxRenderer&) {}} sdFontSystem;
// Font/icon drawing and CompactHeader theme height are external boundaries.
namespace freeink::ui { class GfxRendererTarget; }
namespace CompactHeader {
int height(const ThemeMetrics& metrics) { return std::min(metrics.headerHeight, 67) + 10; }
}
