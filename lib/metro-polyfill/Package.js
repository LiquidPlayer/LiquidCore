/**
 * Copyright (c) 2013-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * 
 * @format
 */

'use strict';var _extends = Object.assign || function (target) {for (var i = 1; i < arguments.length; i++) {var source = arguments[i];for (var key in source) {if (Object.prototype.hasOwnProperty.call(source, key)) {target[key] = source[key];}}}return target;};

const fs = require('fs');
const isAbsolutePath = require('absolute-path');
const path = require('path');








class Package {





  constructor(_ref) {let file = _ref.file;
    this.path = path.resolve(file);
    this._root = path.dirname(this.path);
    this._content = null;
  }

  /**
     * The `browser` field and replacement behavior is specified in
     * https://github.com/defunctzombie/package-browser-field-spec.
     */
  getMain() {
    const json = this.read();

    let main;
    if (typeof json['react-native'] === 'string') {
      main = json['react-native'];
    } else if (typeof json.browser === 'string') {
      main = json.browser;
    } else {
      main = json.main || 'index';
    }

    const replacements = getReplacements(json);
    if (replacements) {
      const variants = [main];
      if (main.slice(0, 2) === './') {
        variants.push(main.slice(2));
      } else {
        variants.push('./' + main);
      }

      for (const variant of variants) {
        const winner =
        replacements[variant] ||
        replacements[variant + '.js'] ||
        replacements[variant + '.json'] ||
        replacements[variant.replace(/(\.js|\.json)$/, '')];

        if (winner) {
          main = winner;
          break;
        }
      }
    }

    /* $FlowFixMe: `getReplacements` doesn't validate the return value. */
    return path.join(this._root, main);
  }

  invalidate() {
    this._content = null;
  }

  redirectRequire(name) {
    const json = this.read();
    const replacements = getReplacements(json);

    if (!replacements) {
      return name;
    }

    if (!isAbsolutePath(name)) {
      const replacement = replacements[name];
      // support exclude with "someDependency": false
      return replacement === false ?
      false :
      /* $FlowFixMe: type of replacements is not being validated */
      replacement || name;
    }

    let relPath = './' + path.relative(this._root, name);
    if (path.sep !== '/') {
      relPath = relPath.replace(new RegExp('\\' + path.sep, 'g'), '/');
    }

    let redirect = replacements[relPath];

    // false is a valid value
    if (redirect == null) {
      redirect = replacements[relPath + '.js'];
      if (redirect == null) {
        redirect = replacements[relPath + '.json'];
      }
    }

    // support exclude with "./someFile": false
    if (redirect === false) {
      return false;
    }

    if (redirect) {
      return path.join(
      this._root,
      /* $FlowFixMe: `getReplacements` doesn't validate the return value. */
      redirect);

    }

    return name;
  }

  read() {
    if (this._content == null) {
      this._content = JSON.parse(fs.readFileSync(this.path, 'utf8'));
      // <HACK> Added for liquidcore_cli - need ability to filter out "browser" replacements in
      // package.json without messing with React Native
      const cfg = global.liquidcore_config;
      const name = this._content.name;
      if (cfg && cfg.filterReplacements) {
        if (typeof cfg.filterReplacements === 'string') cfg.filterReplacements = [ cfg.filterReplacements ];
        if (!cfg.enableReplacements) cfg.enableReplacements = {};
        cfg.filterReplacements.forEach(p => {
          if (typeof cfg.enableReplacements[name] === 'string')
            cfg.enableReplacements[name] = [ cfg.enableReplacements[name] ];
          if (!cfg.enableReplacements[name] || !cfg.enableReplacements[name].includes(p)) {
            delete this._content[p]
          }
        });
      }
      // </HACK>
    }
    return this._content;
  }}


function getReplacements(pkg) {
  let rn = pkg['react-native'];
  let browser = pkg.browser;
  if (rn == null && browser == null) {
    return null;
  }
  // If the field is a string, that doesn't mean we want to redirect the `main`
  // file itself to anything else. See the spec.
  if (rn == null || typeof rn === 'string') {
    rn = {};
  }
  if (browser == null || typeof browser === 'string') {
    browser = {};
  }
  // merge with "browser" as default,
  // "react-native" as override
  return _extends({}, browser, rn);
}

module.exports = Package;