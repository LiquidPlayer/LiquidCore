Node for iOS
------------

The challenge with getting Node.js to work on iOS is an Apple rule.  For whatever reason, Apple will not allow any dynamic
code generation that isn't done through its own JavaScript engine, JavaScriptCore.  So the V8 backend is not allowed. Any 
attempts to use other engines will get your app declined in the App Store.  Unfortunately, Node.js is deeply 
intertwined with V8, which creates a problem.

There were are 3 options:

1. Port V8 to iOS anyway.  Given that it does build for OSX, presumably it wouldn't be too hard to get it to compile and
run.  This would allow everything to work smoothly like it does for Android.  The downside is that it would only be suitable
for apps deployed outside of the App Store.  This is a non-starter, given the goals for the project.

2. Replace the V8 API calls in Node with equivalent calls to JavaScriptCore.  This is doable, although a lot of work.  The
downside is that it isn't scalable.  I've taken great pains to make as few changes to the node codebase as possible so that
when significant newer versions are released, it only takes a weekend to integrate, and to minimize the introduction of bugs.
This would kill the ability to do that.  So it was not a preferred option.

3. Write a V8 -> JSC shim which mimics the V8 API and translates it JavaScriptCore.  This is already 
[done in reverse](https://github.com/LiquidPlayer/LiquidCore/tree/master/LiquidCoreAndroid/src/main/cpp/JSC), as 
I wanted to support React Native running on top of LiquidCore.  React Native is deeply intertwined with JSC, and following
the same principle of modifying as little code as possible, this was the most extensible way to do this.  However,
this was manageable because the JSC API is rather straightforward.  V8 is not.

So the magic contained in the `V82JSC` directory is option 3.  The `v8.h` header and its inlines are supported, but none of
the V8 backend is included.  The vast majority of V8 API tests now pass, and more than enough to support node.

This was much more work than even I originally anticipated, as the V8 inlines make some important assumptions about the
implementation details.  I had to replicate those details to get it to all work unchanged.  So this required, amongst
other things, for me to write my own heap management and garbage collector.  Thus, there may well be bugs.  Additionally,
there are probably a lot of opportunities for optimization.  I have not done any profiling yet, so if you see extreme slowness
anywhere, please file an issue.  Finally, although I implemented perhaps 90% of the V8 API, there are some esoteric functions
that I did not implement, because either the test suite did not cover them or node does not use them.  Attempts to call those
will result in a failed assertion.  If any of those become critical, please also file an issue.

The good news is that the footprint is tiny.  The `NodeConsole` test app weighs in at just 2.3 MB.
