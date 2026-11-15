
int failures = 0;
void check(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    ++failures;
  }
}

struct Fixture {
  GfxRenderer renderer;
  MappedInputManager input;
  AppStoreActivity app{renderer, input};

  Fixture(int w = 480, int h = 800) {
    wifiConnected = false;
    fetches = 0;
    failAllocation = false;
    renderer.w = w;
    renderer.h = h;
    app.onEnter();
  }

  Rect button() {
    app.render(RenderLock{});
    check(renderer.outlines.size() == 1, "offline screen renders one visible connect button");
    check(std::find(renderer.texts.begin(), renderer.texts.end(), "Connect Wi-Fi") != renderer.texts.end(),
          "visible connect button has translated label");
    // Keep exercising the remaining behaviors when the baseline lacks a button.
    return renderer.outlines.empty() ? Rect{49, renderer.h / 2 + 27, renderer.w - 88, 56} : renderer.outlines[0];
  }

  void tap(int x, int y) {
    input.tap(x, y);
    app.loop();
    input.reset();
  }
};

int main() {
  for (auto dimensions : {std::pair{480, 800}, std::pair{800, 480}}) {
    Fixture f(dimensions.first, dimensions.second);
    const Rect b = f.button();
    check(b.x >= f.renderer.safe[3] && b.y >= f.renderer.safe[0] &&
              b.x + b.width <= f.renderer.w - f.renderer.safe[1] &&
              b.y + b.height <= f.renderer.h - f.renderer.safe[2],
          "button within oriented safe area");
    f.tap(b.x + b.width / 2, b.y + b.height / 2);
    check(f.app.children == 1, "visible center tap opens Wi-Fi selection");
    if (f.app.callback) {
      f.app.callback({});
      check(fetches == 0, "cancel does not fetch");
      f.app.render(RenderLock{});
      check(f.renderer.outlines.size() == 1, "cancel redraws connect button");
      f.tap(b.x, b.y);
      check(f.app.children == 2, "cancel is retryable at top-left edge");
    }
    for (auto p : {std::pair{b.x, b.y}, std::pair{b.x + b.width - 1, b.y + b.height - 1}}) {
      Fixture e(dimensions.first, dimensions.second);
      e.tap(p.first, p.second);
      check(e.app.children == 1, "inclusive button edges open selection");
    }
    for (auto p : {std::pair{b.x - 1, b.y + b.height / 2}, std::pair{b.x + b.width, b.y + b.height / 2},
                   std::pair{b.x + b.width / 2, b.y - 1}, std::pair{b.x + b.width / 2, b.y + b.height},
                   std::pair{dimensions.first - 2, dimensions.second - 2}}) {
      Fixture e(dimensions.first, dimensions.second);
      e.tap(p.first, p.second);
      check(e.app.children == 0 && !e.app.finished, "off-button and hidden footer taps ignored");
    }
  }
  {
    Fixture f;
    f.input.confirm = true;
    f.app.loop();
    f.input.reset();
    check(f.app.children == 1, "Confirm opens Wi-Fi selection");
    if (f.app.callback) {
      wifiConnected = true;
      f.app.callback({});
      check(fetches == 0, "successful callback defers network fetch");
      check(f.app.waits == 0, "callback does not wait on rendering");
      f.app.loop();
      check(fetches == 1, "next loop fetches catalog");
    }
  }
  {
    Fixture f;
    const Rect b = f.button();
    f.tap(b.x + b.width / 2, b.y + b.height / 2);
    if (f.app.callback) {
      wifiConnected = true;
      f.app.callback({});
      check(fetches == 0, "touch callback defers fetch");
      f.app.render(RenderLock{});
      check(f.renderer.outlines.empty(), "connected return removes offline action");
      f.app.loop();
      check(fetches == 1, "touch connection advances on next loop");
    }
  }
  {
    Fixture f;
    wifiConnected = true;
    f.app.onEnter();
    f.app.loop();
    check(fetches == 1, "already connected startup fetches catalog");
  }
  {
    Fixture f;
    f.input.back = true;
    f.app.loop();
    check(f.app.finished, "physical Back exits");
  }
  {
    Fixture f;
    f.tap(f.renderer.safe[3] + 10, f.renderer.safe[0] + 20);
    check(f.app.finished, "touch header Back exits");
  }
  {
    Fixture f;
    f.input.touch = false;
    f.app.render(RenderLock{});
    check(f.renderer.outlines.empty(), "button-only device retains hardware hints");
    f.input.confirm = true;
    f.app.loop();
    check(f.app.children == 1, "button-only Confirm connects");
  }
  {
    Fixture f;
    f.input.touch = false;
    f.input.back = true;
    f.app.loop();
    check(f.app.finished, "button-only Back exits");
  }
  {
    Fixture f;
    failAllocation = true;
    f.input.confirm = true;
    f.app.loop();
    f.input.reset();
    check(f.app.children == 0 && fetches == 0, "allocation failure leaves offline screen usable");
    f.button();
    failAllocation = false;
    f.input.confirm = true;
    f.app.loop();
    check(f.app.children == 1, "picker allocation failure can be retried");
  }
  {
    Fixture f;
    wifiConnected = true;
    f.app.loop();
    check(fetches == 0, "connection appearing while offline schedules fetch");
    f.app.loop();
    check(fetches == 1 && f.app.waits == 1, "scheduled fetch renders loading first");
  }
  if (failures) {
    std::fprintf(stderr, "%d behavioral assertions failed\n", failures);
    return 1;
  }
  std::puts("PASS: App Store production render/input/callback behavior "
            "(portrait, landscape, safe areas, edges, cancel, success, navigation)");
}
