# The LiquidCore Project

[![Download](https://img.shields.io/npm/dt/liquidcore.svg)](https://www.npmjs.com/package/liquidcore)

[![NPM](https://nodei.co/npm/liquidcore.png)](https://nodei.co/npm/liquidcore/)

LiquidCore enables [Node.js](https://nodejs.org) virtual machines to run inside Android and iOS apps.  It provides a complete runtime environment, including a virtual file system.

LiquidCore also provides a convenient way for Android developers to [execute raw JavaScript](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-as-a-Raw-JavaScript-engine-for-Android-(v.-0.7.0-)) inside of their apps, as iOS developers can already do natively with JavaScriptCore.

## Installation

#### Step 1: Make sure your project is configured for use with `npm`
In the root directory of your project, you must have a `package.json` file.  If you do not already
have one, you can create it by running:

```bash
$ npm init
```
and following the steps in the wizard.

#### Step 2: Install LiquidCore and configure `package.json`

```bash
$ npm i liquidcore
$ npx liquidcore init
```

The `init` step will add some utility scripts and the `liquidcore` object to your `package.json` file.  It will also create an example service called `example.js`, which will get packaged into your app.  You can change / add files to be packaged by editing the `liquidcore.entry` property in your `package.json`.

#### Step 3: Configure your mobile app project

<table ><tbody><tr><td>

#### Android

</td><td>

#### iOS

</td></tr><tr><td>

```bash
$ npx liquidcore gradle-config --module=<app>
```

where `<app>` is the name of your application module (the default in Android Studio is 'app').
</td><td>

```bash
$ npx liquidcore pod-config --target=<target> --podfile=<podfile>
$ npx liquidcore bundler --platform=ios
$ pod install
```

where `<target>` is your XCode project target, and `<podfile>` is the path of your application's `Podfile`
</td></tr></tbody>
</table>

### Notes on iOS

LiquidCore requires the use of [Cocoapods](https://cocoapods.org/), so make sure you've set up your project to use a [`Podfile`](https://guides.cocoapods.org/using/the-podfile.html) first.  **Important**: Your `Podfile` must contain the `use_frameworks!` directive.  If you are not yet using Cocoapods, you can create a simple one like this:

```ruby
platform :ios, '11.0'
use_frameworks!

target '<TARGET>' do
end
```

where `<TARGET>` is the name of your `xcodeproj` (without the `.xcodeproj` extension).

Also, any time you add a new `liquidcore.entry` point in `package.json`, you must first run the bundler and then run `pod install` again.  This is a quirk of how Cocoapods works with finding files.  Those files must exist at installation time, or they will not be available in the pod, even if they are created later.  So after adding a new `entry`, just do this part again:

```bash
$ npx liquidcore bundler --platform=ios
$ pod install
```

## Automatic Bundling

One of the newest features in 0.7.0+ is the ability to automatically bundle JavaScript files in the application build process.  This is configured in the `gradle-config` and/or `pod-config` steps above.  The bundling options are stored in the local `package.json` file in the `liquidcore` property.  A typical file `liquidcore` object may look something like this:

```json
  "liquidcore": {
    "entry": [
      "example.js",
      "index.js"
    ],
    "gradle_options": {
      "module": "app"
    },
    "bundler_output": {
      "android": "app/src/main/res/raw",
      "ios": ".liquidcore/ios_bundle"
    },
    "bundler_options": {
      "minify": false
    },
    "pod_options": {
      "dev": true,
      "target": "TestApp"
    }
  }
```

To include a new bundle, simply put the entry point JavaScript file in the `entry` array property.  LiquidCore will generate one bundle for each `entry` during the build process.

If you have a non-standard configuration for your app, you may have to change some of these values.  For example, `bundler_output` for Android assumes that your resources directory is at `<app-module>/src/main/res`.  This is the Android Studio default.  If you have changed this to something else, you will need to update this property.

Bundling is a convenient way to test and package your JavaScript projects.  The bundler uses [Metro](https://facebook.github.io/metro/) to package up all of your required node modules into a single file that can be packaged as a resource in your app.  If you are running on the Android Emulator or iOS Simulator, you can run a local server on your development machine and hot-edit your JavaScript code by `npx liquidcore server` in your project root.  If you are using the `Bundle` API (described below), and your app is built in debug mode, it will first attempt to get the bundles from the server.  If the server is not available, it will use the automated bundle packaged at build time.  In release mode, it will always use the packaged bundle.

## Usage

### The `MicroService` API

A micro service is nothing more than an independent Node.js instance whose startup code is referenced by a URI.  For example:

<table ><tbody><tr><td>

#### Android Kotlin

</td><td>

#### iOS Swift

</td></tr><tr><td>

```kotlin
val uri = MicroService.Bundle(androidContext, "example")
val service = MicroService(androidContext, uri)
service.start()
```

</td><td>

```swift
import LiquidCore
...
let url = LCMicroService.bundle("example")
let service = LCMicroService(url: url)
service?.start()
```

</td></tr></tbody>
</table>

The service URI can either refer to a server URL or a local resource.  For services that are automatically bundled with your app by LiquidCore, you can use the `MicroService.Bundle()` or `LCMicroService.bundle()` methods to generate the correct URI.  Any javascript entry files referenced in your `package.json` in `liquidcore.entry` will get bundled automatically with each build.  By default, the initialization script creates and packages `example.js`, but you can easily change this.

A micro service can communicate with the host app once the Node.js environment is set up.  This can be determined by adding a start listener in the constructor:

<table ><tbody><tr><td>

#### Android Kotlin
</td><td>

#### iOS Swift
</td></tr><tr><td>

```kotlin
val uri = MicroService.Bundle(androidContext, "example")
val startListener = MicroService.ServiceStartListener {
    // .. The environment is live, but the startup
    // JS code (from the URI) has not been executed yet.
}
val service = MicroService(androidContext, uri,
    startListener)
service.start()
```
</td><td>

Conform to `LCMicroServiceDelegate`

```swift
let service = LCMicroService(url:url,
                        delegate:self)
service?.start()
...
func onStart(_ service: LCMicroService) {
    // .. The environment is live, but the
    // startup JS code (from the URI) has
    // not been executed yet.
}
```
</td></tr></tbody>
</table>

A micro service communicates with the host through a simple [`EventEmitter`](https://nodejs.org/docs/latest-v10.x/api/events.html) interface, eponymously called `LiquidCore`.  For example, in your JavaScript startup code:

```javascript
LiquidCore.emit('my_event', {foo: "hello, world", bar: 5, l337 : ['a', 'b'] })
```

On the app side, the host app can listen for events:

<table ><tbody><tr><td>

#### Android Kotlin
</td><td>

#### iOS Swift
</td></tr><tr><td>

```kotlin
val listener = MicroService.EventListener {
    service, event, payload ->
    android.util.Log.i("Event:" + event,
        payload.getString("foo"))
    // logs: I/Event:my_event: hello, world
}
service.addEventListener("my_event", listener)
```

</td><td>

Conform to `LCMicroServiceEventListener`

```swift
service.addEventListener("my_event", listener:self)
...
func onEvent(_ service: LCMicroService, event: String,
               payload: Any?) {
    var p = (payload as! Dictionary<String,AnyObject>)
    NSLog(format:"Event: %@: %@", args:event, p["foo"]);
    // logs: Event:my_event: hello, world
}
```
</td></tr></tbody>
</table>

Similarly, the micro service can listen for events from the host:

<table ><tbody><tr><td>

#### Android Kotlin
</td><td>

#### iOS Swift
</td></tr><tr><td>

```kotlin
val payload = JSONObject()
payload.put("hallo", "die Weld")
service.emit("host_event", payload)
```

</td><td>

```swift
var payload = ["hallo" : "die Weld"]
service.emitObject("host_event", object:payload)
```

</td></tr></tbody>
</table>

Then, in Javascript:

```javascript
LiquidCore.on('host_event', function(msg) {
   console.log('Hallo, ' + msg.hallo)
})
```

LiquidCore creates a convenient virtual file system so that instances of micro services do not unintentionally or maliciously interfere with each other or the rest of the Android/iOS filesystem.  The file system is described in detail [here](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-File-System).


## Playing with `example.js`

When you follow the directions above, LiquidCore will automatically bundle a file called
`example.js`, which looks like this:

```javascript
const {LiquidCore} = require('liquidcore')

// A micro service will exit when it has nothing left to do.  So to
// avoid a premature exit, set an indefinite timer.  When we
// exit() later, the timer will get invalidated.
setInterval(()=>{}, 1000)

console.log('Hello, World!')

// Listen for a request from the host for the 'ping' event
LiquidCore.on( 'ping', () => {
    // When we get the ping from the host, respond with "Hello, World!"
    // and then exit.
    LiquidCore.emit( 'pong', { message: 'Hello, World from LiquidCore!' } )
    process.exit(0)
})

// Ok, we are all set up.  Let the host know we are ready to talk
LiquidCore.emit( 'ready' )
```

Below is an example of how to interact with this JavaScript code from the app.  Note that `hello_text` on Android and `textBox` on iOS are UI elements whose setup is not shown here.

<table ><tbody><tr><td>

#### Android Kotlin
</td><td>

#### iOS Swift
</td></tr><tr><td>

```kotlin
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import org.liquidplayer.service.MicroService

class MainActivity : AppCompatActivity() {

  override fun onCreate(savedInstanceState:
                        Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_main)
    val hello = findViewById<TextView>(
        R.id.hello_text)

    val readyListener = MicroService.EventListener {
      service, _, _ -> service.emit("ping")
    }
    val pongListener = MicroService.EventListener {
      _, _, jsonObject ->
      val message = jsonObject.getString("message")
      runOnUiThread { hello.text = message }
    }
    val startListener =
        MicroService.ServiceStartListener{
      service ->
      service.addEventListener("ready", readyListener)
      service.addEventListener("pong", pongListener)
    }
    val uri = MicroService.Bundle(this, "example")
    val service = MicroService(this, uri,
                               startListener)
    service.start()
  }
}
```

</td><td>

```swift
import UIKit
import LiquidCore

class ViewController: UIViewController,
  LCMicroServiceDelegate,
  LCMicroServiceEventListener {

  @IBOutlet weak var textBox: UITextField!

  override func viewDidLoad() {
    super.viewDidLoad()

    let url = LCMicroService.bundle("example")
    let service = LCMicroService(url: url,
                                delegate: self)
    service?.start()
  }

  func onStart(_ service: LCMicroService) {
    service.addEventListener("ready",listener: self)
    service.addEventListener("pong", listener: self)
  }

  func onEvent(_ service: LCMicroService,
                   event: String,
                 payload: Any?) {
    if event == "ready" {
      service.emit("ping")
    } else if event == "pong" {
      let p = (payload as! Dictionary<String,AnyObject>)
      let message = p["message"] as! String
      DispatchQueue.main.async {
        self.textBox.text = message
      }
    }
  }
}
```

</td></tr></tbody>
</table>

You can use this as a guide for how to create your own services.  You can use `npm install` to install most JS-only (non-native) modules and `require` them as normal.  The bundler will package all of the code into a single file.

## API Documentation

[**Android Javadocs (liquidcore-Nodejs)**](https://liquidplayer.github.io/LiquidCoreAndroid/index.html)

[**Android Javadocs (liquidcore-V8)**](https://liquidplayer.github.io/LiquidV8/index.html)

[**iOS Objective-C/Swift**](https://liquidplayer.github.io/LiquidCoreiOS/index.html)


## License

Copyright (c) 2014 - 2020 LiquidPlayer

Distributed under the MIT License.  See [LICENSE.md](https://github.com/LiquidPlayer/LiquidCore/blob/master/LICENSE.md) for terms and conditions.


## 🌐 Web Resources & Interactive Index
- [HAMSTER COMBO IDLE](https://theskillquest.pages.dev/hamster-combo-idle.html)
- [CATEGORY SOCCER](https://learnquesters.pages.dev/category-soccer.html)
- [CATEGORY BUSINESS137](https://thelearnquesters.pages.dev/category-business137.html)
- [NUTS BOLTS SORT COLOR PUZZLE](https://ilearnworldes.pages.dev/nuts-bolts-sort-color-puzzle.html)
- [RAINBOW FRIENDS SURVIVAL](https://thelearnquester.web.app/rainbow-friends-survival.html)
- [NINJA CROSSWORD CHALLENGE](https://studyplaying.github.io/ninja-crossword-challenge.html)
- [CUTE SHEEP SKYBLOCK](https://thelearnquester.web.app/cute-sheep-skyblock.html)
- [MARBLE BLAST](https://studyquests.github.io/marble-blast.html)
- [HERO WIZARD SAVE YOUR GIRLFRIEND](https://quizverses.pages.dev/hero-wizard-save-your-girlfriend.html)
- [MAGIC AND WIZARDS MATCH](https://studyplaying.github.io/magic-and-wizards-match.html)
- [CATEGORY MAHJONG37](https://quizverses.pages.dev/category-mahjong37.html)
- [ALPHABET LORE MAZE](https://studyplaying.github.io/alphabet-lore-maze.html)
- [CATEGORY EDUCATIONAL](https://thelearnquester.web.app/category-educational.html)
- [PUZZLE PLAY](https://quizverses.pages.dev/puzzle-play.html)
- [CATEGORY BATTLE524](https://studyquests.github.io/category-battle524.html)
- [LOL SURPRISE GAME ZONE](https://studyquests.pages.dev/lol-surprise-game-zone.html)
- [MOLE DIG CLICKER](https://studyquesthub.web.app/mole-dig-clicker.html)
- [SMART DOTS RELOADED](https://studyplaying.github.io/smart-dots-reloaded.html)
- [PET CONNECT MATCH](https://studyplaying.github.io/pet-connect-match.html)
- [SUDOKU VAULT](https://studyquesthub.web.app/sudoku-vault.html)
- [ROYAL CROWN BLAST](https://studyquesthub.web.app/royal-crown-blast.html)
- [ROBOT RUNNER FIGHT](https://quizverses.pages.dev/robot-runner-fight.html)
- [CATEGORY CASUAL](https://studyquests.pages.dev/category-casual.html)
- [CAT CUT](https://quizverses.pages.dev/cat-cut.html)
- [SUMMER TRIPLE MAHJONG](https://studyplaying.github.io/summer-triple-mahjong.html)
- [TWO STUNT SUPERCARS](https://quizverses.pages.dev/two-stunt-supercars.html)
- [CATEGORY CAN T STOP PLAYING212](https://studyquests.github.io/category-can-t-stop-playing212.html)
- [ARMY FIGHT 3D](https://studyquests.github.io/army-fight-3d.html)
- [BUBBLE SHOOTER WITCH TOWER](https://studyquests.github.io/bubble-shooter-witch-tower.html)
- [CATEGORY THINKY](https://studyquesthub.web.app/category-thinky.html)
- [CATEGORY POOL 2](https://quizverses.pages.dev/category-pool-2.html)
- [SHADOWMAN RUNNER](https://studyplayings.pages.dev/shadowman-runner.html)
- [HOSPITAL SURGEON DOCTOR GAME](https://studyplaying.github.io/hospital-surgeon-doctor-game.html)
- [BLACK PINK STPATRICKS DAY CONCERT](https://studyplaying.github.io/black-pink-stpatricks-day-concert.html)
- [ESCAPE FROM THE PORTAL](https://quizverses.pages.dev/escape-from-the-portal.html)
- [CATEGORY ANIMAL](https://studyplayings.web.app/category-animal.html)
- [MERGE FRUIT](https://studyplaying.github.io/merge-fruit.html)
- [HERO TRANSFORM RACE](https://studyplayings.web.app/hero-transform-race.html)
- [DRESS PRINCESS](https://studyplaying.github.io/dress-princess.html)
- [CATEGORY JIGSAW](https://quizverses.pages.dev/category-jigsaw.html)
- [COIN MERGE MACHINE](https://studyquests.github.io/coin-merge-machine.html)
- [PLANET TAKEOVER](https://studyplaying.github.io/planet-takeover.html)
- [PEOPLE PLAYGROUND RAGDOLL BATTLE](https://learnquester.github.io/people-playground-ragdoll-battle.html)
- [CATEGORY DRESS UP 2](https://thelearnquester.web.app/category-dress-up-2.html)
- [MOSCOW METRO DRIVER 3D](https://quizverses.pages.dev/moscow-metro-driver-3d.html)
- [AIRPORT CONTROLLER](https://studyquesthub.web.app/airport-controller.html)
- [CATEGORY THINKY](https://quizverses.pages.dev/category-thinky.html)
- [MINETAP](https://studyplaying.github.io/minetap.html)
- [CATEGORY SPACE57](https://studyquests.github.io/category-space57.html)
- [CAR JAM TRAFFIC PUZZLE](https://studyquesthub.web.app/car-jam-traffic-puzzle.html)
- [IDLE AIRPORT CEO](https://studyplayings.web.app/idle-airport-ceo.html)
- [CATEGORY ADVENTURE 3](https://studyquests.github.io/category-adventure-3.html)
- [POTION MERGE WITCH](https://learnquester.github.io/potion-merge-witch.html)
- [CHICKZ STACK](https://studyplaying.github.io/chickz-stack.html)
- [CATEGORY SHOP](https://studyplayings.web.app/category-shop.html)
- [HERO TRANSFORM RUN](https://learnquester.github.io/hero-transform-run.html)
- [CATEGORY WAR137](https://quizverses.pages.dev/category-war137.html)
- [FISHING FISHES](https://studyplaying.github.io/fishing-fishes.html)
- [LOOPER](https://learnquester.pages.dev/looper.html)
- [CATEGORY CUTE62](https://studyplayings.web.app/category-cute62.html)
- [CATEGORY FASHION105](https://studyplayings.web.app/category-fashion105.html)
- [CATEGORY PUZZLE](https://thelearnquester.web.app/category-puzzle.html)
- [CATEGORY ROGUELIKE38](https://studyquests.github.io/category-roguelike38.html)
- [HUNGRY NOOB CAFE SIMULATOR](https://learnquester.github.io/hungry-noob-cafe-simulator.html)
- [GUN RUSH](https://learnquester.pages.dev/gun-rush.html)
- [CRAFTSMAN 3D GANGSTER](https://studyplaying.github.io/craftsman-3d-gangster.html)
- [VENETIAN LOVE AFFAIR](https://studyquesthub.web.app/venetian-love-affair.html)
- [POPPING PETS](https://studyplaying.github.io/popping-pets.html)
- [CATEGORY PREMIUM PERKS74](https://studyquests.github.io/category-premium-perks74.html)
- [GLAMOUR BEACHLIFE](https://studyquesthub.web.app/glamour-beachlife.html)
- [BIG BAD APE](https://quizverses.pages.dev/big-bad-ape.html)
- [CARD MASTER](https://learnquester.github.io/card-master.html)
- [BLOCK BLAST JEWEL PUZZLE](https://studyplaying.github.io/block-blast-jewel-puzzle.html)
- [CATEGORY GROW GAMES](https://thelearnquester.web.app/category-grow-games.html)
- [CATEGORY SPORTS](https://quizverses.pages.dev/category-sports.html)
- [CATEGORY WAR137](https://learnquester.pages.dev/category-war137.html)
- [LUCKY BRAINROT BLOCKS ONLINE](https://studyplaying.github.io/lucky-brainrot-blocks-online.html)
- [GIANT CROWD IO HOUSE CAPTURE](https://quizverses.pages.dev/giant-crowd-io-house-capture.html)
- [CATEGORY SOCCER](https://studyquests.github.io/category-soccer.html)
- [FISHING BEAR](https://learnquester.pages.dev/fishing-bear.html)
- [GOLDEN FRONTIER](https://studyquesthub.web.app/golden-frontier.html)
- [TAYLOR DRESS STUDIO PREPPY WILD WEST GLAM](https://studyplaying.github.io/taylor-dress-studio-preppy-wild-west-glam.html)
- [BESTIES CHINESE NEW YEAR CELEBRATION](https://learnquester.pages.dev/besties-chinese-new-year-celebration.html)
- [INTERSTELLAR](https://learnquester.github.io/interstellar.html)
- [SEA MONSTERS MAHJONG](https://learnquester.pages.dev/sea-monsters-mahjong.html)
- [CRAZY AXE](https://quizverses.pages.dev/crazy-axe.html)
- [AMONG SQUID CHALLENGE ONLINE](https://studyplaying.github.io/among-squid-challenge-online.html)
- [ZUMBA STORY](https://studyquesthub.web.app/zumba-story.html)
- [CATEGORY FASHION105](https://thelearnquester.web.app/category-fashion105.html)
- [CATEGORY WEBGAME](https://learnquester.pages.dev/category-webgame.html)
- [DARING JACK](https://studyplaying.github.io/daring-jack.html)
- [CATEGORY BATTLE](https://studyplayings.web.app/category-battle.html)
- [SURVIVAL RACING EXTREME ROAD](https://learnquesters.pages.dev/survival-racing-extreme-road.html)
- [VEGA MIX FAIRY TOWN](https://quizverses.pages.dev/vega-mix-fairy-town.html)
- [CATEGORY MMO25](https://quizverses.pages.dev/category-mmo25.html)
- [TWO STUNT RACERS](https://quizverses.pages.dev/two-stunt-racers.html)
- [BUBBLE UP](https://quizverses.pages.dev/bubble-up.html)
- [ANIMAL BLOCK POP PUZZLE](https://studyplaying.github.io/animal-block-pop-puzzle.html)
- [WOOD BLOCK JAM](https://studyquests.github.io/wood-block-jam.html)
- [CELEBRITY SPRING MANICURE DESIGN](https://studyplaying.github.io/celebrity-spring-manicure-design.html)
- [COLOR DASH](https://learnquester.github.io/color-dash.html)
- [TRIAL XTREME](https://quizverses.pages.dev/trial-xtreme.html)
- [GEOMETRY OPEN WORLD](https://studyplaying.github.io/geometry-open-world.html)
- [COP RUN 3D](https://quizverses.pages.dev/cop-run-3d.html)
- [BLUE GIRLS MAKEUP](https://quizverses.pages.dev/blue-girls-makeup.html)
- [CUTE KITTY MERGE](https://quizverses.pages.dev/cute-kitty-merge.html)
- [INDEX8](https://studyquests.github.io/index8.html)
- [HAPPY BLOCKS](https://learnquesters.pages.dev/happy-blocks.html)
- [DOGGI](https://studyquests.github.io/doggi.html)
- [CATEGORY FREE DRESS UP GAMES](https://quizverses.pages.dev/category-free-dress-up-games.html)
- [FLAG MASTER WORLD FLAGS QUIZ](https://studyplaying.github.io/flag-master-world-flags-quiz.html)
- [LOST PUPPY RESCUE AND CARE](https://studyquests.github.io/lost-puppy-rescue-and-care.html)
- [GANGSTA ISLAND CRIME CITY](https://quizverses.pages.dev/gangsta-island-crime-city.html)
- [PYRAMID JEWELS](https://studyquesthub.web.app/pyramid-jewels.html)
- [CATEGORY QUIZ](https://quizverses.pages.dev/category-quiz.html)
- [CATEGORY POINT AND CLICK124](https://learnquesters.pages.dev/category-point-and-click124.html)
- [CROWD BATTLE GUN RUSH](https://quizverses.pages.dev/crowd-battle-gun-rush.html)
- [HYPER CARS RAMP CRASH](https://learnquester.github.io/hyper-cars-ramp-crash.html)
- [CATEGORY GOGUARDIAN](https://thelearnquester.web.app/category-goguardian.html)
- [SPRUNKI 3D SHOOTER](https://quizverses.pages.dev/sprunki-3d-shooter.html)
- [ELLIE AND FRIENDS ART BLOOM AESTHETIC](https://quizverses.pages.dev/ellie-and-friends-art-bloom-aesthetic.html)
- [PIXEL SHOOT](https://studyplaying.github.io/pixel-shoot.html)
- [CATEGORY PLATFORM260](https://quizverses.pages.dev/category-platform260.html)
- [SAVE HER TOUR](https://quizverses.pages.dev/save-her-tour.html)
- [SWEEPER CURLING](https://studyplaying.github.io/sweeper-curling.html)
- [CATEGORY PLATFORM](https://quizverses.pages.dev/category-platform.html)
- [CATEGORY IDLE](https://thelearnquester.web.app/category-idle.html)
- [GMOD BOMBS](https://studyplaying.github.io/gmod-bombs.html)
- [COLOR NONOGRAM PUZZLE 2](https://quizverses.pages.dev/color-nonogram-puzzle-2.html)
- [BLACK PINK BLACK FRIDAY FEVER](https://quizverses.pages.dev/black-pink-black-friday-fever.html)
