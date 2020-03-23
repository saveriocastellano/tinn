
	
var self = this;


(function() {

var isWindows = process.platform === 'win32';

var util = {
	isString: function(arg) {
		return typeof arg === 'string';	
	},
	isObject: function(arg) {
		return typeof arg === 'object' && arg !== null;
	}
}
// resolves . and .. elements in a path array with directory names there
// must be no slashes or device names (c:\) in the array
// (so also no leading and trailing slashes - it does not distinguish
// relative and absolute paths)
function normalizeArray(parts, allowAboveRoot) {
  var res = [];
  for (var i = 0; i < parts.length; i++) {
    var p = parts[i];

    // ignore empty parts
    if (!p || p === '.')
      continue;

    if (p === '..') {
      if (res.length && res[res.length - 1] !== '..') {
        res.pop();
      } else if (allowAboveRoot) {
        res.push('..');
      }
    } else {
      res.push(p);
    }
  }

  return res;
}

// returns an array with empty elements removed from either end of the input
// array or the original array if no elements need to be removed
function trimArray(arr) {
  var lastIndex = arr.length - 1;
  var start = 0;
  for (; start <= lastIndex; start++) {
    if (arr[start])
      break;
  }

  var end = lastIndex;
  for (; end >= 0; end--) {
    if (arr[end])
      break;
  }

  if (start === 0 && end === lastIndex)
    return arr;
  if (start > end)
    return [];
  return arr.slice(start, end + 1);
}

// Regex to split a windows path into three parts: [*, device, slash,
// tail] windows-only
var splitDeviceRe =
    /^([a-zA-Z]:|[\\\/]{2}[^\\\/]+[\\\/]+[^\\\/]+)?([\\\/])?([\s\S]*?)$/;

// Regex to split the tail part of the above into [*, dir, basename, ext]
var splitTailRe =
    /^([\s\S]*?)((?:\.{1,2}|[^\\\/]+?|)(\.[^.\/\\]*|))(?:[\\\/]*)$/;

var win32 = {};

// Function to split a filename into [root, dir, basename, ext]
function win32SplitPath(filename) {
  // Separate device+slash from tail
  var result = splitDeviceRe.exec(filename),
      device = (result[1] || '') + (result[2] || ''),
      tail = result[3] || '';
  // Split the tail into dir, basename and extension
  var result2 = splitTailRe.exec(tail),
      dir = result2[1],
      basename = result2[2],
      ext = result2[3];
  return [device, dir, basename, ext];
}

function win32StatPath(path) {
  var result = splitDeviceRe.exec(path),
      device = result[1] || '',
      isUnc = !!device && device[1] !== ':';
  return {
    device: device,
    isUnc: isUnc,
    isAbsolute: isUnc || !!result[2], // UNC paths are always absolute
    tail: result[3]
  };
}

function normalizeUNCRoot(device) {
  return '\\\\' + device.replace(/^[\\\/]+/, '').replace(/[\\\/]+/g, '\\');
}

// path.resolve([from ...], to)
win32.resolve = function() {
  var resolvedDevice = '',
      resolvedTail = '',
      resolvedAbsolute = false;

  for (var i = arguments.length - 1; i >= -1; i--) {
    var path;
    if (i >= 0) {
      path = arguments[i];
    } else if (!resolvedDevice) {
      path = process.cwd();
    } else {
      // Windows has the concept of drive-specific current working
      // directories. If we've resolved a drive letter but not yet an
      // absolute path, get cwd for that drive. We're sure the device is not
      // an unc path at this points, because unc paths are always absolute.
      path = process.env['=' + resolvedDevice];
      // Verify that a drive-local cwd was found and that it actually points
      // to our drive. If not, default to the drive's root.
      if (!path || path.substr(0, 3).toLowerCase() !==
          resolvedDevice.toLowerCase() + '\\') {
        path = resolvedDevice + '\\';
      }
    }

    // Skip empty and invalid entries
    if (!util.isString(path)) {
      throw new TypeError('Arguments to path.resolve must be strings');
    } else if (!path) {
      continue;
    }

    var result = win32StatPath(path),
        device = result.device,
        isUnc = result.isUnc,
        isAbsolute = result.isAbsolute,
        tail = result.tail;

    if (device &&
        resolvedDevice &&
        device.toLowerCase() !== resolvedDevice.toLowerCase()) {
      // This path points to another device so it is not applicable
      continue;
    }

    if (!resolvedDevice) {
      resolvedDevice = device;
    }
    if (!resolvedAbsolute) {
      resolvedTail = tail + '\\' + resolvedTail;
      resolvedAbsolute = isAbsolute;
    }

    if (resolvedDevice && resolvedAbsolute) {
      break;
    }
  }

  // Convert slashes to backslashes when `resolvedDevice` points to an UNC
  // root. Also squash multiple slashes into a single one where appropriate.
  if (isUnc) {
    resolvedDevice = normalizeUNCRoot(resolvedDevice);
  }

  // At this point the path should be resolved to a full absolute path,
  // but handle relative paths to be safe (might happen when process.cwd()
  // fails)

  // Normalize the tail path
  resolvedTail = normalizeArray(resolvedTail.split(/[\\\/]+/),
                                !resolvedAbsolute).join('\\');

  return (resolvedDevice + (resolvedAbsolute ? '\\' : '') + resolvedTail) ||
         '.';
};


win32.normalize = function(path) {
  var result = win32StatPath(path),
      device = result.device,
      isUnc = result.isUnc,
      isAbsolute = result.isAbsolute,
      tail = result.tail,
      trailingSlash = /[\\\/]$/.test(tail);

  // Normalize the tail path
  tail = normalizeArray(tail.split(/[\\\/]+/), !isAbsolute).join('\\');

  if (!tail && !isAbsolute) {
    tail = '.';
  }
  if (tail && trailingSlash) {
    tail += '\\';
  }

  // Convert slashes to backslashes when `device` points to an UNC root.
  // Also squash multiple slashes into a single one where appropriate.
  if (isUnc) {
    device = normalizeUNCRoot(device);
  }

  return device + (isAbsolute ? '\\' : '') + tail;
};


win32.isAbsolute = function(path) {
  return win32StatPath(path).isAbsolute;
};

win32.join = function() {
  var paths = [];
  for (var i = 0; i < arguments.length; i++) {
    var arg = arguments[i];
    if (!util.isString(arg)) {
      throw new TypeError('Arguments to path.join must be strings');
    }
    if (arg) {
      paths.push(arg);
    }
  }

  var joined = paths.join('\\');

  // Make sure that the joined path doesn't start with two slashes, because
  // normalize() will mistake it for an UNC path then.
  //
  // This step is skipped when it is very clear that the user actually
  // intended to point at an UNC path. This is assumed when the first
  // non-empty string arguments starts with exactly two slashes followed by
  // at least one more non-slash character.
  //
  // Note that for normalize() to treat a path as an UNC path it needs to
  // have at least 2 components, so we don't filter for that here.
  // This means that the user can use join to construct UNC paths from
  // a server name and a share name; for example:
  //   path.join('//server', 'share') -> '\\\\server\\share\')
  if (!/^[\\\/]{2}[^\\\/]/.test(paths[0])) {
    joined = joined.replace(/^[\\\/]{2,}/, '\\');
  }

  return win32.normalize(joined);
};


// path.relative(from, to)
// it will solve the relative path from 'from' to 'to', for instance:
// from = 'C:\\orandea\\test\\aaa'
// to = 'C:\\orandea\\impl\\bbb'
// The output of the function should be: '..\\..\\impl\\bbb'
win32.relative = function(from, to) {
  from = win32.resolve(from);
  to = win32.resolve(to);

  // windows is not case sensitive
  var lowerFrom = from.toLowerCase();
  var lowerTo = to.toLowerCase();

  var toParts = trimArray(to.split('\\'));

  var lowerFromParts = trimArray(lowerFrom.split('\\'));
  var lowerToParts = trimArray(lowerTo.split('\\'));

  var length = Math.min(lowerFromParts.length, lowerToParts.length);
  var samePartsLength = length;
  for (var i = 0; i < length; i++) {
    if (lowerFromParts[i] !== lowerToParts[i]) {
      samePartsLength = i;
      break;
    }
  }

  if (samePartsLength == 0) {
    return to;
  }

  var outputParts = [];
  for (var i = samePartsLength; i < lowerFromParts.length; i++) {
    outputParts.push('..');
  }

  outputParts = outputParts.concat(toParts.slice(samePartsLength));

  return outputParts.join('\\');
};


win32._makeLong = function(path) {
  // Note: this will *probably* throw somewhere.
  if (!util.isString(path))
    return path;

  if (!path) {
    return '';
  }

  var resolvedPath = win32.resolve(path);

  if (/^[a-zA-Z]\:\\/.test(resolvedPath)) {
    // path is local filesystem path, which needs to be converted
    // to long UNC path.
    return '\\\\?\\' + resolvedPath;
  } else if (/^\\\\[^?.]/.test(resolvedPath)) {
    // path is network UNC path, which needs to be converted
    // to long UNC path.
    return '\\\\?\\UNC\\' + resolvedPath.substring(2);
  }

  return path;
};


win32.dirname = function(path) {
  var result = win32SplitPath(path),
      root = result[0],
      dir = result[1];

  if (!root && !dir) {
    // No dirname whatsoever
    return '.';
  }

  if (dir) {
    // It has a dirname, strip trailing slash
    dir = dir.substr(0, dir.length - 1);
  }

  return root + dir;
};


win32.basename = function(path, ext) {
  var f = win32SplitPath(path)[2];
  // TODO: make this comparison case-insensitive on windows?
  if (ext && f.substr(-1 * ext.length) === ext) {
    f = f.substr(0, f.length - ext.length);
  }
  return f;
};


win32.extname = function(path) {
  return win32SplitPath(path)[3];
};


win32.format = function(pathObject) {
  if (!util.isObject(pathObject)) {
    throw new TypeError(
        "Parameter 'pathObject' must be an object, not " + typeof pathObject
    );
  }

  var root = pathObject.root || '';

  if (!util.isString(root)) {
    throw new TypeError(
        "'pathObject.root' must be a string or undefined, not " +
        typeof pathObject.root
    );
  }

  var dir = pathObject.dir;
  var base = pathObject.base || '';
  if (!dir) {
    return base;
  }
  if (dir[dir.length - 1] === win32.sep) {
    return dir + base;
  }
  return dir + win32.sep + base;
};


win32.parse = function(pathString) {
  if (!util.isString(pathString)) {
    throw new TypeError(
        "Parameter 'pathString' must be a string, not " + typeof pathString
    );
  }
  var allParts = win32SplitPath(pathString);
  if (!allParts || allParts.length !== 4) {
    throw new TypeError("Invalid path '" + pathString + "'");
  }
  return {
    root: allParts[0],
    dir: allParts[0] + allParts[1].slice(0, -1),
    base: allParts[2],
    ext: allParts[3],
    name: allParts[2].slice(0, allParts[2].length - allParts[3].length)
  };
};


win32.sep = '\\';
win32.delimiter = ';';


// Split a filename into [root, dir, basename, ext], unix version
// 'root' is just a slash, or nothing.
var splitPathRe =
    /^(\/?|)([\s\S]*?)((?:\.{1,2}|[^\/]+?|)(\.[^.\/]*|))(?:[\/]*)$/;
var posix = {};


function posixSplitPath(filename) {
  return splitPathRe.exec(filename).slice(1);
}


// path.resolve([from ...], to)
// posix version
posix.resolve = function() {
  var resolvedPath = '',
      resolvedAbsolute = false;

  for (var i = arguments.length - 1; i >= -1 && !resolvedAbsolute; i--) {
    var path = (i >= 0) ? arguments[i] : process.cwd();

    // Skip empty and invalid entries
    if (!util.isString(path)) {
      throw new TypeError('Arguments to path.resolve must be strings');
    } else if (!path) {
      continue;
    }

    resolvedPath = path + '/' + resolvedPath;
    resolvedAbsolute = path[0] === '/';
  }

  // At this point the path should be resolved to a full absolute path, but
  // handle relative paths to be safe (might happen when process.cwd() fails)

  // Normalize the path
  resolvedPath = normalizeArray(resolvedPath.split('/'),
                                !resolvedAbsolute).join('/');

  return ((resolvedAbsolute ? '/' : '') + resolvedPath) || '.';
};

// path.normalize(path)
// posix version
posix.normalize = function(path) {
  var isAbsolute = posix.isAbsolute(path),
      trailingSlash = path && path[path.length - 1] === '/';

  // Normalize the path
  path = normalizeArray(path.split('/'), !isAbsolute).join('/');

  if (!path && !isAbsolute) {
    path = '.';
  }
  if (path && trailingSlash) {
    path += '/';
  }

  return (isAbsolute ? '/' : '') + path;
};

// posix version
posix.isAbsolute = function(path) {
  return path.charAt(0) === '/';
};

// posix version
posix.join = function() {
  var path = '';
  for (var i = 0; i < arguments.length; i++) {
    var segment = arguments[i];
    if (!util.isString(segment)) {
      throw new TypeError('Arguments to path.join must be strings');
    }
    if (segment) {
      if (!path) {
        path += segment;
      } else {
        path += '/' + segment;
      }
    }
  }
  return posix.normalize(path);
};


// path.relative(from, to)
// posix version
posix.relative = function(from, to) {
  from = posix.resolve(from).substr(1);
  to = posix.resolve(to).substr(1);

  var fromParts = trimArray(from.split('/'));
  var toParts = trimArray(to.split('/'));

  var length = Math.min(fromParts.length, toParts.length);
  var samePartsLength = length;
  for (var i = 0; i < length; i++) {
    if (fromParts[i] !== toParts[i]) {
      samePartsLength = i;
      break;
    }
  }

  var outputParts = [];
  for (var i = samePartsLength; i < fromParts.length; i++) {
    outputParts.push('..');
  }

  outputParts = outputParts.concat(toParts.slice(samePartsLength));

  return outputParts.join('/');
};


posix._makeLong = function(path) {
  return path;
};


posix.dirname = function(path) {
  var result = posixSplitPath(path),
      root = result[0],
      dir = result[1];

  if (!root && !dir) {
    // No dirname whatsoever
    return '.';
  }

  if (dir) {
    // It has a dirname, strip trailing slash
    dir = dir.substr(0, dir.length - 1);
  }

  return root + dir;
};


posix.basename = function(path, ext) {
  var f = posixSplitPath(path)[2];
  // TODO: make this comparison case-insensitive on windows?
  if (ext && f.substr(-1 * ext.length) === ext) {
    f = f.substr(0, f.length - ext.length);
  }
  return f;
};


posix.extname = function(path) {
  return posixSplitPath(path)[3];
};


posix.format = function(pathObject) {
  if (!util.isObject(pathObject)) {
    throw new TypeError(
        "Parameter 'pathObject' must be an object, not " + typeof pathObject
    );
  }

  var root = pathObject.root || '';

  if (!util.isString(root)) {
    throw new TypeError(
        "'pathObject.root' must be a string or undefined, not " +
        typeof pathObject.root
    );
  }

  var dir = pathObject.dir ? pathObject.dir + posix.sep : '';
  var base = pathObject.base || '';
  return dir + base;
};


posix.parse = function(pathString) {
  if (!util.isString(pathString)) {
    throw new TypeError(
        "Parameter 'pathString' must be a string, not " + typeof pathString
    );
  }
  var allParts = posixSplitPath(pathString);
  if (!allParts || allParts.length !== 4) {
    throw new TypeError("Invalid path '" + pathString + "'");
  }
  allParts[1] = allParts[1] || '';
  allParts[2] = allParts[2] || '';
  allParts[3] = allParts[3] || '';

  return {
    root: allParts[0],
    dir: allParts[0] + allParts[1].slice(0, -1),
    base: allParts[2],
    ext: allParts[3],
    name: allParts[2].slice(0, allParts[2].length - allParts[3].length)
  };
};


posix.sep = '/';
posix.delimiter = ':';

var path;
if (isWindows)
  path = win32;
else /* posix */
  path = posix;

function Module(id, parent) {
  this.id = id;
  this.exports = {};
  this.parent = parent;
  if (parent && parent.children) {
    parent.children.push(this);
  }

  this.filename = null;
  this.loaded = false;
  this.children = [];
}


// If obj.hasOwnProperty has been overridden, then calling
// obj.hasOwnProperty(prop) will break.
// See: https://github.com/joyent/node/issues/1707
function hasOwnProperty(obj, prop) {
  return Object.prototype.hasOwnProperty.call(obj, prop);
}


Module._extensions = {};
Module._cache = {};
var modulePaths = [];
Module.globalPaths = [];
Module._pathCache = {};

// bootstrap main module.

Module._initPaths = function() {
  var isWindows = process.platform === 'win32';

  if (isWindows) {
    var homeDir = process.env.USERPROFILE;
  } else {
    var homeDir = process.env.HOME;
  }

  var paths = [path.resolve(process.execPath, '..', '..', 'lib', 'tinn')];

  if (homeDir) {
    paths.unshift(path.resolve(homeDir, '.tinn_libraries'));
    paths.unshift(path.resolve(homeDir, '.tinn_modules'));
  }

  var tinnPath = process.env['TINN_PATH'];
  if (tinnPath) {
    paths = tinnPath.split(path.delimiter).concat(paths);
  }

  modulePaths = paths;

  // clone as a read-only copy, for introspection.
  Module.globalPaths = modulePaths.slice(0);
};



// check if the directory is a package.json dir
var packageMainCache = {};

function readPackage(requestPath) {
  if (hasOwnProperty(packageMainCache, requestPath)) {
    return packageMainCache[requestPath];
  }
  var json;
  try {
    var jsonPath = path.resolve(requestPath, 'package.json');
    json = read(jsonPath);
  } catch (e) {
    return false;
  }

  try {
    var pkg = packageMainCache[requestPath] = JSON.parse(json).main;
  } catch (e) {
    e.path = jsonPath;
    e.message = 'Error parsing ' + jsonPath + ': ' + e.message;
    throw e;
  }
  return pkg;
}


function tryPackage(requestPath, exts) {
  var pkg = readPackage(requestPath);

  if (!pkg) return false;

  var filename = path.resolve(requestPath, pkg);
  return tryFile(filename) || tryExtensions(filename, exts) ||
         tryExtensions(path.resolve(filename, 'index'), exts);
}

Module._realpathCache = {};

function tryFile(requestPath) {
  if( path.isFileReadable(requestPath)) {
	return requestPath;
  }else {
	return false;
  }  
}

// given a path check a the file exists with any of the set extensions
function tryExtensions(p, exts) {
  for (var i = 0, EL = exts.length; i < EL; i++) {
    var filename = tryFile(p + exts[i]);

    if (filename) {
      return filename;
    }
  }
  return false;
}

Module._findPath = function(request, paths) {
  var exts = Object.keys(Module._extensions);

  if (request.charAt(0) === '/') {
    paths = [''];
  }

  var trailingSlash = (request.slice(-1) === '/');

  var cacheKey = JSON.stringify({request: request, paths: paths});
  if (Module._pathCache[cacheKey]) {
    return Module._pathCache[cacheKey];
  }

  // For each path
  for (var i = 0, PL = paths.length; i < PL; i++) {
    var basePath = path.resolve(paths[i], request);
    var filename;

    if (!trailingSlash) {
      // try to join the request to the path
      filename = tryFile(basePath);
	  
      if (!filename && !trailingSlash) {
        // try it with each of the extensions
        filename = tryExtensions(basePath, exts);
      }	  

    }
    if (!filename) {
      filename = tryPackage(basePath, exts);
    }
    if (!filename) {
      // try it with each of the extensions at "index"
      filename = tryExtensions(path.resolve(basePath, 'index'), exts);
    }
	if (!filename){
	  var basePath = path.resolve(basePath, request);
      filename = tryFile(basePath);
	  
      if (!filename) {
        // try it with each of the extensions
        filename = tryExtensions(basePath, exts);
      }		  
	}
	
    if (filename) {
      Module._pathCache[cacheKey] = filename;
      return filename;
    }
  }
  return false;
};

Module._tinnModulePaths = function(from) {
  // guarantee that 'from' is absolute.
  from = path.resolve(from);

  // note: this approach *only* works when the path is guaranteed
  // to be absolute.  Doing a fully-edge-case-correct path.split
  // that works on both Windows and Posix is non-trivial.
  var splitRe = process.platform === 'win32' ? /[\/\\]/ : /\//;
  var paths = [];
  var parts = from.split(splitRe);

  for (var tip = parts.length - 1; tip >= 0; tip--) {
    // don't search in .../tinn_modules/tinn_modules
    if (parts[tip] === 'tinn_modules') continue;
    var dir = parts.slice(0, tip + 1).concat('tinn_modules').join(path.sep);
    paths.push(dir);
  }
  return paths;
};


Module._resolveLookupPaths = function(request, parent) {
  var start = request.substring(0, 2);
  if (start !== './' && start !== '..') {
    var paths = modulePaths;
    if (parent) {
      if (!parent.paths) parent.paths = [];
      paths = parent.paths.concat(paths);
    }
    return [request, paths];
  }

  // with --eval, parent.id is not set and parent.filename is null
  if (!parent || !parent.id || !parent.filename) {
    // make require('./path/to/foo') work - normally the path is taken
    // from realpath(__filename) but with eval there is no filename
    var mainPaths = ['.'].concat(modulePaths);
    mainPaths = Module._tinnModulePaths('.').concat(mainPaths);
    return [request, mainPaths];
  }

  // Is the parent an index module?
  // We can assume the parent has a valid extension,
  // as it already has been accepted as a module.
  var isIndex = /^index\.\w+?$/.test(path.basename(parent.filename));
  var parentIdPath = isIndex ? parent.id : path.dirname(parent.id);
  var id = path.resolve(parentIdPath, request);
  
  // make sure require('./path') and require('path') get distinct ids, even
  // when called from the toplevel js file
  if (parentIdPath === '.' && id.indexOf('/') === -1) {
    id = './' + id;
  }

  return [id, [path.dirname(parent.filename)]];
};


Module._resolveFilename = function(request, parent) {
  var resolvedModule = Module._resolveLookupPaths(request, parent);
  var id = resolvedModule[0];
  var paths = resolvedModule[1];
  var filename = Module._findPath(request, paths);
  if (!filename) {
    var err = new Error("Cannot find module '" + request + "' in: " + JSON.stringify(paths));
    err.code = 'MODULE_NOT_FOUND';
    throw err;
  }
  return filename;
};



Module._load = function(request, parent) {
  var filename; 
  if (typeof(parent)=='undefined' && request=='<d8>'){ 
    filename = isWindows ? 'd8.exe' : 'd8';
  } else if (typeof(parent)!='undefined') {
	filename = Module._resolveFilename(request, parent);  
  } else {
	filename = request;  
  }

  var cachedModule = Module._cache[filename];
  if (cachedModule) {
    return cachedModule.exports;
  }
  var module = new Module(filename, parent);

  if (typeof(parent)=='undefined') {
    process.mainModule = module;
	module.loaded = true;
    module.id = '.';
	module.filename = filename;
	module.paths = Module._tinnModulePaths(path.dirname(filename));
	return;
  }

  Module._cache[filename] = module;

  var hadException = true;

  try {
    module.load(filename);
    hadException = false;
  } finally {
    if (hadException) {
      delete Module._cache[filename];
    }
  }

  return module.exports;
};

Module.prototype.load = function(filename) {
  this.filename = filename;
  this.paths = Module._tinnModulePaths(path.dirname(filename));

  var extension = path.extname(filename) || '.js';
  if (!Module._extensions[extension]) extension = '.js';
  Module._extensions[extension](this, filename);
  this.loaded = true;
};




Module.prototype.require = function(path) {
  return Module._load(path, this);
};

Module.prototype._compile = function(content, filename) {
  var self = this;
  // remove shebang
  content = content.replace(/^\#\!.*/, '');

  function require(path) {
    return self.require(path);
  }

  require.resolve = function(request) {
    return Module._resolveFilename(request, self);
  };

  require.main = process.mainModule;
  require.extensions = Module._extensions;
  require.cache = Module._cache;

  var dirname = path.dirname(filename);
  
  var wrapper = '(function (exports, require, module, __filename, __dirname) { ' + content + '\n});';
  var compiledWrapper = run(wrapper, filename);
    
  var args = [self.exports, require, self, filename, dirname];
  var res = compiledWrapper.apply(self.exports, args);
  return res;
};


function stripBOM(content) {
  // Remove byte order marker. This catches EF BB BF (the UTF-8 BOM)
  // because the buffer-to-string conversion in `fs.readFileSync()`
  // translates it to FEFF, the UTF-16 BOM.
  if (content.charCodeAt(0) === 0xFEFF) {
    content = content.slice(1);
  }
  return content;
}

Module._extensions['.js'] = function(module, filename) {
  var content = read(filename);
  module._compile(stripBOM(content), filename);
};

path.isFileReadable = process.isFileReadable;
delete process.isFileReadable;
self.path = path;
self.require = function(what) {
	return Module._load(what, typeof(process.mainModule)=='object' ? process.mainModule : undefined);
}

})();

if (typeof(process.mainModule)=='undefined') {
	process.mainModule = '<d8>';
}
require(process.mainModule);
delete this.self;
delete this.testRunner;
var global = this;
var window = this;	
var exports = {};
var module = process.mainModule;
var __filename = module.filename;
var __dirname = path.dirname(module.filename);

process.version = version();

if (process.platform === 'win32') {
	function fixPath(p) { return p.split('\\').join('\\\\'); }	
	Http._openSocket = Http.openSocket;
	Http.openSocket = function(addr, nginxPort) {
		nginxPort = nginxPort ? nginxPort : 80;
		var nginxDir = process.cwd() + '\\nginx';
		var nginx = nginxDir + '\\nginx.exe'; 
		var nginxConf = nginxDir + '\\nginx.conf'; 
		var nginxConfTpl = nginxDir + '\\nginx.conf.tpl'; 
		
		if (OS.isFileAndReadable(nginx)) {
			OS.exec([fixPath(process.env['SystemRoot']+'\\System32\\taskkill.exe'), '/F', '/IM', 'nginx.exe', '2>', 'nul']);
			var listenAddr = addr.indexOf(":")==0? '127.0.0.1'+addr : addr;
			if (OS.isFileAndReadable(nginxConfTpl)) {
				var tpl = OS.readFile(nginxConfTpl);
					tpl = tpl.replace('%ADDR%',listenAddr);
				tpl = tpl.replace('%PORT%',nginxPort);
				OS.writeFile(nginxConf, tpl);
			}
			new Worker('OS.exec(["'+fixPath(nginx)+'","-c","nginx.conf","-p","'+fixPath(nginxDir)+'"]);', {type:'string'});
			print("nginx listening on port " + nginxPort);
		}
		Http._openSocket(addr);
	}
}


var pkg = new function() {
	
	this._parseHttpResponse = function(res) {
		var hdrParts = res.split('\r\n\r\n');
		if (hdrParts.length >=2 && hdrParts[0].indexOf('HTTP/')==0 && hdrParts[1].indexOf('HTTP/')==0) {
			//we have the connect headers.. let's suppress it :)
			res = hdrParts.slice(1).join('\r\n\r\n');			
		}
		var hdrEnd = res.indexOf('\r\n\r\n');
		var headers = res.substr(0, hdrEnd);
		var isChunked = (headers.indexOf('Transfer-Encoding: chunked')!=-1);
		if (!isChunked) {
			return res.substring(hdrEnd+4);
		} 	
		var i = hdrEnd+4;
		var str = '';
		var body = '';
		var partLength = -1;
		while(i < res.length) {
			var c1 = res.charAt(i);
			var c2 = res.charAt(i+1);
			i++;
			if (c1 == '\r' && c2 == '\n') {
				if (partLength==-1) {
					partLength = parseInt(str, 16);
					str = '';	
					continue;
				}
				body += str;
				str = '';	
				partLength = -1;
				continue;
			}
			str += c1;
		}
		return body;
	}
	
	this._searchAndGetAll = function(what) {
		//print("search " + what);
		var url = 'https://api.github.com/search/repositories?q='+ what;
		if (this._verbose) print("url=" + url);
		var retryCount = 0;
		var retry = true;
		do {
			if (retryCount > 0 && this._verbose) print("retrying...");
			var res = Http.request(url);
			if (res.result == 0) {
				try {
					var obj = JSON.parse(this._parseHttpResponse(res.response));
					if (!obj.items) {
						print("error response from repository: " + JSON.stringify(obj,null,4));
						return null;
					}
					return obj;
				} catch(e) {
					//not a json
					print("invalid reply from repository");//: " + e.message+ ": " + res.response);
				}
			} else {
				//http failed
				print("error sending request to repository");
			}		
		} while(retry && retryCount++ <3)
		return null;
	}
	
	this._searchAndGetFirst = function(what, id) {
		//14527085
		//var q = what;
		//if (id && !isNaN(parseInt(id))) q += '+id:' + id;
		var obj = this._searchAndGetAll(what);
		if (!obj) return;
		if (obj.items == 0) {
			print("Project not found: " + what + (id ? ' '+id : ''));
			return;
		}
		if (isNaN(parseInt(id))) {
			return obj.items[0];
		} else {
			var item = obj.items.filter(function(el){
				return (el.id == id);
			})
			return item;
		}
	}
	
	this.info = function(what, id) {
		var obj = this._searchAndGetFirst(what, id);
		if (!obj) return;
		print(JSON.stringify(obj, null,4));
		
	} 
	
	this._getInstalldir = function(isGlobal){
		var tinnPath = process.env['TINN_PATH'];
		
		if (!isGlobal) {
			return OS.cwd();
		} else if (tinnPath && OS.isDirAndReadable(tinnPath)){
			return tinnPath;
		} else {
			return process.cwd();
		}
	}
	
	this._stripFlag = function(flag) { 
		var found = this._args.indexOf('--'+flag);
		if (found==-1) found = this._args.indexOf('-'+flag.charAt(0));
		if (found!=-1) {
			this._args.splice(found, 1);
			return true;
		}
		return false;
	}
	
	this._stripOtherFlags = function(){
		var args = [];
		for (var i=0;i<this._args.length; i++) { 
			if (this._args[i].charAt(0)!='-') args.push(this._args[i]);
		}
		this._args = args;
	}
	
	this._getGit = function(){ 
		var file;
		try {
			if (process.platform=='win32') {
				file = OS.exec(['where','git', '2>', 'nul']).output.split('\n')[0];
			} else {
				file = OS.exec(['which','git']).output.split('\n')[0];
			}
		} catch(e) {}
		if (!OS.isFileAndReadable(file)) {
			print("'git' not found. Make sure 'git' is installed and in PATH");
			return null; 
		}
		return (process.platform=='win32') ? '"'+file+'"' : file;
	}
	//git clone -b <tag_name> --single-branch <repo_url> [<dest_dir>] 
	this.install = function(depWhat, depVersion, depInstPath) {
		var tag = '';
		var pkg;
		var instDir;
		var obj;
		
		if (typeof(depWhat)=='undefined') {
			var isGlobal = this._stripFlag('global');
			this._stripOtherFlags();
			if (this._args.length == 0) {
				print("no package given... checking package.json");
				return;
			}
			instDir = path.resolve(this._getInstalldir(isGlobal), 'tinn_modules');
			
			pkg = this._args[0].split("@");		
			if (pkg.length>1) tag = pkg[1];
			this._args[0] = pkg = pkg[0];		
		
			obj = this._searchAndGetFirst.apply(this, this._args);		
			if (!obj) return;
			//print("tag=" + tag);
		} else {
			var searchArgs = depWhat.split(' ');
			obj = this._searchAndGetFirst.apply(this, searchArgs);		
			if (!obj) {
				print("dependency not found: " + depWhat);
				return;
			}	
			tag = depVersion;
			instDir = depInstPath;
			pkg = searchArgs[0];
			
			//REMOVE this:
			//todo: handle ^/tilde
			//https://api.github.com/repos/saveriocastellano/tinn/tags
			if (tag.charAt(0)=='^') tag = tag.substring(1);
		}		
		
		print("Installing " + obj.name + " in " + instDir);
		if (!OS.isDirAndReadable(instDir)){					
			OS.mkdir(instDir);
			print("created install directory: " + instDir);
		}
		
		var pkgInstDir = path.resolve(instDir, pkg);
		if (OS.isDirAndReadable(pkgInstDir)) {
			print(pkg + ' is already installed in ' + pkgInstDir);
			return;
		}		
		var git = this._getGit();
		var cmd = [git, 'clone'];
		if (tag!='') {
			cmd = cmd.concat(['-b', tag, '--single-branch']);
		}
		cmd.push(obj.git_url);
		cmd.push(pkgInstDir);
		var res = OS.exec(cmd);
		
		
		if (OS.isDirAndReadable(pkgInstDir)) {
			//deps?
			var pkgJsonPath = path.resolve(pkgInstDir, 'package.json');
			
			//REMOVE ME
			if (!OS.isFileAndReadable(pkgJsonPath) && obj.name == 'tinn_web') {				
				var json = JSON.stringify({
					"dependencies": {
						"color-convert": "1.9.0"
					}				
				});
				OS.writeFile(pkgJsonPath, json);
			}
			//END
			
			if (OS.isFileAndReadable(pkgJsonPath)) {
				var pkgJson = JSON.parse(OS.readFile(pkgJsonPath));
				print("handle " + obj.name + " deps..");
				for (var dep in pkgJson.dependencies) {
					print("Installing dependency: " + dep + ' ' + pkgJson.dependencies[dep]);
					this.install(dep, pkgJson.dependencies[dep], path.resolve(pkgInstDir, 'tinn_modules'));
				}
			} else{
				print("no deps");
			}
			
			print('\n'+pkg + ' ready.');
		}
	}
	
	
	this._removeTree = function(dir){ 
		var files = OS.listDir(dir);
		for (var i=0; i<files.length; i++){
			//print("file: " + files[i]);
			var file = path.resolve(dir, files[i]);
			if (OS.isFileAndReadable(file)) {
				OS.unlink(file);
			} else {
				this._removeTree(file);
			}
		}
		OS.rmdir(dir);
	}
	
	this.remove = function(){
		var isGlobal = this._stripFlag('global');
		var isSave = this._stripFlag('save');
		this._stripOtherFlags();
		
		if (this._args.length == 0) {
			print("no package given...");
			return;
		}	
		
		var pkg = this._args[0];
		var instDir = path.resolve(this._getInstalldir(isGlobal), 'tinn_modules');
		var pkgInstDir = path.resolve(instDir, pkg);
		if (!OS.isDirAndReadable(pkgInstDir)) {
			print("Package " + pkg + " not found in " + instDir);
			return;
		}
		print("Removing " + pkg + " from " + instDir);
		this._removeTree(pkgInstDir);
	}
	
	this.uninstall = this.remove;
	
	this.search = function(what){ 
		var obj = this._searchAndGetAll(what);
		print("Found "  +obj.total_count + " projects: "); 
		for (var i=0; i<obj.items.length;i++) {
			var proj = obj.items[i];
			//print(JSON.stringify(obj.items[i],null,4));
			print(''+proj.name + " " + proj.id + " - " + proj.description);
			//break;
		}	
	}
	
	this._setVerbose = function(v){
		this._verbose = v;
	}
	
	this._setArgs = function(args){
		this._args = args;
	}
	
}

	
if (typeof(pkg[arguments[0]])!='undefined') {
	var args = arguments.slice(1);
	pkg._setArgs(args);
	pkg._setVerbose(args.indexOf('-v')!=-1);
	pkg[arguments[0]].apply(pkg);
}	
		
	
	

	
