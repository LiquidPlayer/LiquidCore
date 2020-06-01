const Class = function () {
  var options = Object.create({
    Source: Object,
    config: {},
    buildArgs: []
  });

  function checkOption(option) {
    var key = "config";
    if (option instanceof Function)
      key = "Source";
    else if (option instanceof Array)
      key = "buildArgs";
    else if (option instanceof Object)
      key = "config";
    else
      throw new Error("Invalid configuration option.");
    if (options.hasOwnProperty(key))
      throw new Error("Duplicated configuration option: " + key + ".");
    options[key] = option;
  }

  for (var index = 0, length = arguments.length; index < length; ++index)
    checkOption(arguments[index]);

  var Source = options.Source,
    config = options.config,
    buildArgs = options.buildArgs;

  return (Source.extend || Class.extend).call(Source, config, buildArgs);
};

Class.factory = function () {
  var Source = this;
  return function () {
    var instance = this;
    if (instance.build instanceof Function)
      instance.build.apply(instance, arguments);
    if (instance.init instanceof Function)
      instance.init.apply(instance, arguments);
  };
};

Class.extend = function (config, buildArgs) {
  var Source = this;
  if (!config)
    config = {};
  var Subject;
  if ((config.prototype instanceof Object) && config.prototype.constructor !== Object)
    Subject = config.prototype.constructor;
  else if (config.factory instanceof Function)
    Subject = config.factory.call(Source);
  Subject = (Source.clone || Class.clone).call(Source, Subject, buildArgs);
  (Subject.merge || Class.merge).call(Subject, config);
  return Subject;
};

Class.prototype.extend = function (config, buildArgs) {
  var subject = this;
  var instance = (subject.clone || Class.prototype.clone).apply(subject, buildArgs);
  (instance.merge || Class.prototype.merge).call(instance, config);
  return instance;
};

Class.clone = function (Subject, buildArgs) {
  var Source = this;
  if (!(Subject instanceof Function))
    Subject = (Source.factory || Class.factory).call(Source);
  Subject.prototype = (Source.prototype.clone || Class.prototype.clone).apply(Source.prototype, buildArgs || []);
  Subject.prototype.constructor = Subject;
  for (var staticProperty in Source)
    if (staticProperty !== "prototype")
      Subject[staticProperty] = Source[staticProperty];
  return Subject;
};

Class.prototype.clone = function () {
  var subject = this;
  var instance = Object.create(subject);
  if (instance.build instanceof Function)
    instance.build.apply(instance, arguments);
  return instance;
};

Class.merge = function (config) {
  var Subject = this;
  for (var staticProperty in config)
    if (staticProperty !== "prototype")
      Subject[staticProperty] = config[staticProperty];
  if (config.prototype instanceof Object)
    (Subject.prototype.merge || Class.prototype.merge).call(Subject.prototype, config.prototype);
  return Subject;
};

Class.prototype.merge = function (config) {
  var subject = this;
  for (var property in config)
    if (property !== "constructor")
      subject[property] = config[property];
  return subject;
};

Class.absorb = function (config) {
  var Subject = this;
  for (var staticProperty in config)
    if (staticProperty !== "prototype" && (Subject[staticProperty] === undefined || Subject[staticProperty] === Function.prototype[staticProperty]))
      Subject[staticProperty] = config[staticProperty];
  if (config.prototype instanceof Object)
    (Subject.prototype.absorb || Class.prototype.absorb).call(Subject.prototype, config.prototype);
  return Subject;
};

Class.prototype.absorb = function (config) {
  var subject = this;
  for (var property in config)
    if (property !== "constructor" && (subject[property] === undefined || subject[property] === Object.prototype[property]))
      subject[property] = config[property];
  return subject;
};

Class.getAncestor = function () {
  var Source = this;
  if (Source !== Source.prototype.constructor)
    return Source.prototype.constructor;
};

Class.newInstance = function () {
  var Subject = this;
  var instance = Object.create(this.prototype);
  Subject.apply(instance, arguments);
  return instance;
};

// abstractMethod
const abstractMethod = function abstractMethod() {
  throw new Error("Not implemented.");
};

const cache = function (fn) {
  var called = false,
      store;

  if (!(fn instanceof Function)) {
    called = true;
    store = fn;
    //delete(fn);
  }

  return function () {
    if (!called) {
      called = true;
      store = fn.apply(this, arguments);
      //delete(fn);
    }
    return store;
  };
};

const eachCombination = function eachCombination(alternativesByDimension, callback, combination) {
  if (!combination)
    combination = [];
  if (combination.length < alternativesByDimension.length) {
    var alternatives = alternativesByDimension[combination.length];
    for (var index in alternatives) {
      combination[combination.length] = alternatives[index];
      eachCombination(alternativesByDimension, callback, combination);
      --combination.length;
    }
  }
  else
    callback.apply(null, combination);
}

const capability = function() { return false; }

const AbstractFrameStringSource = Class(Object, {
  prototype: {
    captureFrameStrings: function (frameShifts) {
      var error = this.createError();
      frameShifts.unshift(this.captureFrameStrings);
      frameShifts.unshift(this.createError);
      var capturedFrameStrings = this.getFrameStrings(error);

      var frameStrings = capturedFrameStrings.slice(frameShifts.length),
        functionValues = [];

      if (capability("arguments.callee.caller")) {
        var capturedFunctionValues = [
          this.createError,
          this.captureFrameStrings
        ];
        try {
          var aCaller = arguments.callee;
          while (aCaller = aCaller.caller)
            capturedFunctionValues.push(aCaller);
        }
        catch (useStrictError) {
        }
        functionValues = capturedFunctionValues.slice(frameShifts.length);
      }
      return {
        frameStrings: frameStrings,
        functionValues: functionValues
      };
    },
    getFrameStrings: function (error) {
      var message = error.message || "";
      var name = error.name || "";
      var stackString = this.getStackString(error);
      if (stackString === undefined)
        return;
      var stackStringChunks = stackString.split("\n");
      var fromPosition = 0;
      var toPosition = stackStringChunks.length;
      if (this.hasHeader)
        fromPosition += name.split("\n").length + message.split("\n").length - 1;
      if (this.hasFooter)
        toPosition -= 1;
      return stackStringChunks.slice(fromPosition, toPosition);
    },
    createError: abstractMethod,
    getStackString: abstractMethod,
    hasHeader: undefined,
    hasFooter: undefined
  }
});

const FrameStringSourceCalibrator = Class(Object, {
  prototype: {
    calibrateClass: function (FrameStringSource) {
      return this.calibrateMethods(FrameStringSource) && this.calibrateEnvelope(FrameStringSource);
    },
    calibrateMethods: function (FrameStringSource) {
      try {
        eachCombination([[
          function (message) {
            return new Error(message);
          },
          function (message) {
            try {
              throw new Error(message);
            }
            catch (error) {
              return error;
            }
          }
        ], [
          function (error) {
            return error.stack;
          },
          function (error) {
            return error.stacktrace;
          }
        ]], function (createError, getStackString) {
          if (getStackString(createError()))
            throw {
              getStackString: getStackString,
              createError: createError
            };
        });
      } catch (workingImplementation) {
        Class.merge.call(FrameStringSource, {
          prototype: workingImplementation
        });
        return true;
      }
      return false;
    },
    calibrateEnvelope: function (FrameStringSource) {
      var getStackString = FrameStringSource.prototype.getStackString;
      var createError = FrameStringSource.prototype.createError;
      var calibratorStackString = getStackString(createError("marker"));
      var calibratorFrameStrings = calibratorStackString.split("\n");
      Class.merge.call(FrameStringSource, {
        prototype: {
          hasHeader: /marker/.test(calibratorFrameStrings[0]),
          hasFooter: calibratorFrameStrings[calibratorFrameStrings.length - 1] === ""
        }
      });
      return true;
    }
  }
});

const FrameStringSource = {
  getClass: cache(function () {
    var FrameStringSource;
    if (FrameStringSource)
      return FrameStringSource;
    FrameStringSource = Class(AbstractFrameStringSource, {});
    var calibrator = new FrameStringSourceCalibrator();
    if (!calibrator.calibrateClass(FrameStringSource))
      throw new Error("Cannot read Error.prototype.stack in this environment.");
    return FrameStringSource;
  }),
  getInstance: cache(function () {
    var FrameStringSource = this.getClass();
    var instance = new FrameStringSource();
    return instance;
  })
};

class Frame {
  constructor(config) {
    if (config && config instanceof Object) {
      this.frameString = config.frameString;
      this.functionValue = config.functionValue;
      var fr = /(.*)@(.*):([0-9]*):([0-9]*)/.exec(String(this.frameString));
      this.functionName = (fr && fr[1]) || 'anonymous';
      this.fileName = (fr && fr[2]) || '[code]';
      this.lineNumber = fr && parseInt(fr[3]);
      this.columnNumber = fr && parseInt(fr[4]);
    }
  }
  toString () { return this.frameString; }
  getThis() { abstractMethod() }
  getTypeName() { abstractMethod() }
  getFunction() { return this.functionValue; }
  getFunctionName() { return this.functionName; }
  getMethodName() { return this.functionName; }
  getFileName() { return this.fileName; }
  getLineNumber() { return this.lineNumber; }
  getColumnNumber() { return this.columnNumber; }
  getEvalOrigin() { return false; }
  isTopLevel() { abstractMethod() }
  isEval() { return false; }
  isNative() { return false; }
  isConstructor() { abstractMethod() }
};

const classFrameStringParser = Class(Object, {
  prototype: {
    stackParser: null,
    frameParser: null,
    locationParsers: null,
    constructor: function (options) {
      Class.prototype.merge.call(this, options);
    },
    getFrames: function (frameStrings, functionValues) {
      var frames = [new Frame({}), new Frame({})];
      for (var index = 0, length = frameStrings.length; index < length; ++index)
        frames[index] = this.getFrame(frameStrings[index], functionValues[index]);
      return frames;
    },
    getFrame: function (frameString, functionValue) {
      var config = {
        frameString: frameString,
        functionValue: functionValue
      };
      return new Frame(config);
    }
  }
});

const FrameStringParser = {
  getClass: cache(function () {
    return classFrameStringParser;
  }),
  getInstance: cache(function () {
    var FrameStringParser = this.getClass();
    var instance = new FrameStringParser();
    return instance;
  })
};

Error.captureStackTrace = function captureStackTrace(throwable, terminator) {
  var warnings;
  var frameShifts = [
    captureStackTrace
  ];
  if (terminator) {
    // additional frames can come here if arguments.callee.caller is supported
    // otherwise it is hard to identify the terminator
    frameShifts.push(terminator);
  }
  var captured = FrameStringSource.getInstance().captureFrameStrings(frameShifts);
  Object.defineProperties(throwable, {
    stack: {
      configurable: true,
      get: cache(function () {
        var frames = FrameStringParser.getInstance().getFrames(captured.frameStrings, captured.functionValues);
        return (Error.prepareStackTrace || prepareStackTrace)(throwable, frames, warnings);
      })
    },
    cachedStack: {
      configurable: true,
      writable: true,
      enumerable: false,
      value: true
    }
  });
};

Error.getStackTrace = function (throwable) {
  if (throwable.cachedStack)
    return throwable.stack;
  var frameStrings = FrameStringSource.getInstance().getFrameStrings(throwable),
    frames = [],
    warnings;
  if (frameStrings)
    frames = FrameStringParser.getInstance().getFrames(frameStrings, []);
  else
    warnings = [
      "The stack is not readable by unthrown errors in this environment."
    ];
  var stack = (Error.prepareStackTrace || prepareStackTrace)(throwable, frames, warnings);
  if (frameStrings)
    try {
      Object.defineProperties(throwable, {
        stack: {
          configurable: true,
          writable: true,
          enumerable: false,
          value: stack
        },
        cachedStack: {
          configurable: true,
          writable: true,
          enumerable: false,
          value: true
        }
      });
    } catch (nonConfigurableError) {
    }
  return stack;
};

const prepareStackTrace = function (throwable, frames, warnings) {
  var string = "";
  string += throwable.name || "Error";
  string += ": " + (throwable.message || "");
  if (warnings instanceof Array)
    for (var warningIndex in warnings) {
      var warning = warnings[warningIndex];
      string += "\n   # " + warning;
    }
  for (var frameIndex in frames) {
    var frame = frames[frameIndex];
    string += "\n   at " + frame.toString();
  }
  return string;
};
