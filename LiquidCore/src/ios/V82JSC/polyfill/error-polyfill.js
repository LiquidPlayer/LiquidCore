/* This is adapted from error-polyfill (https://github.com/inf3rno/error-polyfill).  Since
 * we will always have ES6 available, removed the unnecessary stuff.
 */

(function (global, factory) {
	typeof exports === 'object' && typeof module !== 'undefined' ? factory() :
	typeof define === 'function' && define.amd ? define(factory) :
	(factory());
}(this, (function () { 'use strict';

    function eachCombination(alternativesByDimension, callback, combination) {
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

    function prepareStackTrace(throwable, frames, warnings) {
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
        getThis() { throw new Error("absract method"); }
        getTypeName() { throw new Error("absract method"); }
        getFunction() { return this.functionValue; }
        getFunctionName() { return this.functionName; }
        getMethodName() { throw new Error("absract method"); }
        getFileName() { return this.fileName; }
        getLineNumber() { return this.lineNumber; }
        getColumnNumber() { return this.columnNumber; }
        getEvalOrigin() { throw new Error("absract method"); }
        isTopLevel() { throw new Error("absract method"); }
        isEval() { throw new Error("absract method"); }
        isNative() { throw new Error("absract method"); }
        isConstructor() { throw new Error("absract method"); }
    };

    class FrameStringParser {
        constructor(options) {
            this.stackParser = null;
            this.frameParser = null;
            this.locationParsers = null;
        }
        getFrames(frameStrings, functionValues) {
            var frames = [];
            for (var index = 0, length = frameStrings.length; index < length; ++index)
                frames[index] = this.getFrame(frameStrings[index], functionValues[index]);
            return frames;
        }
        getFrame(frameString, functionValue) {
            var config = {
                frameString: frameString,
                functionValue: functionValue
            };
            return new Frame(config);
        }
    };

    class AbstractFrameStringSource {
        constructor() {
            this.hasHeader = undefined;
            this.hasFooter = undefined;
        }
        captureFrameStrings(frameShifts) {
            var error = this.createError();
            frameShifts.unshift(this.captureFrameStrings);
            frameShifts.unshift(this.createError);
            var capturedFrameStrings = this.getFrameStrings(error);

            var frameStrings = capturedFrameStrings.slice(frameShifts.length),
                functionValues = [];

            return {
                frameStrings: frameStrings,
                functionValues: functionValues
            };
        }
        getFrameStrings(error) {
            var message = error.message || "";
            var name = error.name || "";
            var stackString = this.getStackString(error);
            console.log("stackString")
            console.log(stackString)
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
        }
        createError() { throw new Error("absract method"); }
        getStackString() { throw new Error("absract method"); }
    };

    class FrameStringSourceCalibrator {
        calibrateClass(FrameStringSource) {
            return this.calibrateMethods(FrameStringSource) && this.calibrateEnvelope(FrameStringSource);
        }
        calibrateMethods(FrameStringSource) {
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
                Object.assign(FrameStringSource.prototype, workingImplementation);
                return true;
            }
            return false;
        }
        calibrateEnvelope(FrameStringSource) {
            var getStackString = FrameStringSource.prototype.getStackString;
            var createError = FrameStringSource.prototype.createError;
            var calibratorStackString = getStackString(createError("marker"));
            var calibratorFrameStrings = calibratorStackString.split("\n");
            this.hasHeader = /marker/.test(calibratorFrameStrings[0]);
            this.hasFooter = calibratorFrameStrings[calibratorFrameStrings.length - 1] === "";
            return true;
        }
    };

    class FrameStringSource extends AbstractFrameStringSource {};
    var calibrator = new FrameStringSourceCalibrator();
    if (!calibrator.calibrateClass(FrameStringSource))
        throw new Error("Cannot read Error.prototype.stack in this environment.");

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
        var captured = new FrameStringSource().captureFrameStrings(frameShifts);
        Object.defineProperties(throwable, {
            stack: {
                configurable: true,
                get: function () {
                    var frames = new FrameStringParser().getFrames(captured.frameStrings, captured.functionValues);
                    return (Error.prepareStackTrace || prepareStackTrace)(throwable, frames, warnings);
                }
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
        var frameStrings = new FrameStringSource().getFrameStrings(throwable),
            frames = [],
            warnings;
        if (frameStrings)
            frames = new FrameStringParser().getFrames(frameStrings, []);
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

})));
