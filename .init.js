
	
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

  var paths = [path.resolve(OS.cwd(), "tinn_modules")];///*fix*/, '..', '..', 'lib', 'tinn')];
  if (homeDir) {
    paths.unshift(path.resolve(homeDir, '.tinn_libraries'));
    paths.unshift(path.resolve(homeDir, '.tinn_modules'));
  }

  var tinnPath = process.env['TINN_PATH'];
  if (tinnPath) {
    paths = tinnPath./*split(path.delimiter)*/concat(paths);
  }

  modulePaths = paths;

  // clone as a read-only copy, for introspection.
  Module.globalPaths = modulePaths.slice(0);
};

Module._initPaths();

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
	
	this._cc = new function() {;
	
		this.prefix = '\x1b['

		this.up = function up (num) {
		  return this.prefix + (num || '') + 'A'
		}

		this.down = function down (num) {
		  return this.prefix + (num || '') + 'B'
		}

		this.forward = function forward (num) {
		  return this.prefix + (num || '') + 'C'
		}

		this.back = function back (num) {
		  return this.prefix + (num || '') + 'D'
		}

		this.nextLine = function nextLine (num) {
		  return this.prefix + (num || '') + 'E'
		}

		this.previousLine = function previousLine (num) {
		  return this.prefix + (num || '') + 'F'
		}

		this.horizontalAbsolute = function(num) {
		  if (num == null) throw new Error('horizontalAboslute requires a column to position to')
		  return this.prefix + num + 'G'
		}

		this.eraseData = function eraseData () {
		  return this.prefix + 'J'
		}

		this.eraseLine = function eraseLine () {
		  return this.prefix + 'K'
		}

		this.goto = function (x, y) {
		  return this.prefix + y + ';' + x + 'H'
		}

		this.gotoSOL = function () {
		  return '\r'
		}

		this.beep = function () {
		  return '\x07'
		}

		this.hideCursor = function hideCursor () {
		  return this.prefix + '?25l'
		}

		this.showCursor = function showCursor () {
		  return this.prefix + '?25h'
		}

		this.colors = {
		  reset: 0,
		// styles
		  bold: 1,
		  italic: 3,
		  underline: 4,
		  inverse: 7,
		// resets
		  stopBold: 22,
		  stopItalic: 23,
		  stopUnderline: 24,
		  stopInverse: 27,
		// colors
		  white: 37,
		  black: 30,
		  blue: 34,
		  cyan: 36,
		  green: 32,
		  magenta: 35,
		  red: 31,
		  yellow: 33,
		  bgWhite: 47,
		  bgBlack: 40,
		  bgBlue: 44,
		  bgCyan: 46,
		  bgGreen: 42,
		  bgMagenta: 45,
		  bgRed: 41,
		  bgYellow: 43,

		  grey: 90,
		  brightBlack: 90,
		  brightRed: 91,
		  brightGreen: 92,
		  brightYellow: 93,
		  brightBlue: 94,
		  brightMagenta: 95,
		  brightCyan: 96,
		  brightWhite: 97,

		  bgGrey: 100,
		  bgBrightBlack: 100,
		  bgBrightRed: 101,
		  bgBrightGreen: 102,
		  bgBrightYellow: 103,
		  bgBrightBlue: 104,
		  bgBrightMagenta: 105,
		  bgBrightCyan: 106,
		  bgBrightWhite: 107
		}

		this.color = function color (colorWith) {
		  if (arguments.length !== 1 || !Array.isArray(colorWith)) {
			colorWith = Array.prototype.slice.call(arguments)
		  }
		  var self = this;
		  return this.prefix + colorWith.map(function(c){return self.colorNameToCode(c);}).join(';') + 'm'
		}

		this.colorNameToCode = function(color) {
		  if (this.colors[color] != null) return this.colors[color]
		  throw new Error('Unknown color or style name: ' + color)
		}
	}

	this.GIT_URL = 'https://api.github.com';
	
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
	
	this._gitRequest = function(url) {

		this._vprint("git request " + url);
		var retryCount = 0;
		var retry = true;
		do {
			
			var obj = null;
			var res = Http.request(url);
			if (res.result == 0) {
				try {
					obj = null;
					obj = JSON.parse(this._parseHttpResponse(res.response));
				} catch(e) {
					this._wprint("invalid reply from repository");
				}
			} else {
				//http failed
				this._wprint("error sending request to repository");				
			}
			var doSleep = false;
			if (obj) {
				if (typeof(obj.message)=='undefined' || obj.message.indexOf('API rate limit')!=0) {
					return obj;
				} else {
					doSleep = true;
					this._wprint(obj.message);
				}
			}		
			if (doSleep) {
				var sl;
				if (retryCount > 8) {
					sl = 10000;
				} else if (retryCount > 6) {
					sl = 5000;
				} else if (retryCount > 4) {
					sl = 2500;
				} else if (retryCount > 2) {
					sl = 1000;
				} else {
					sl = 200;
				}
				this._wprint("retrying in " + sl + 'ms...');
				OS.sleep(sl);
			}
		} while(retry && retryCount++ <10);	
	}
	
	this._searchAndGetAll = function(what) {
		var url = this.GIT_URL + '/search/repositories?q='+ what;		
		var obj = this._gitRequest(url);
		if (!obj) return null;
		if (!obj.items) {
			print("error response from repository: " + JSON.stringify(obj,null,4));
			return null;
		}		
		return obj;
	}
	
	this._searchAndGetFirst = function(what, id) {
		var obj = this._searchAndGetAll(what);
		if (!obj) return;
		if (obj.items == 0) {
			this._eprint("Project not found: " + what + (id ? ' '+id : ''));
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
	
	this._vprint = function(txt){ 
		if (this._verbose) print(txt);
	}
	
	this._eprint = function(txt){ 
		print(this._cc.color('white','bgRed', 'bold') + 'ERROR' + this._cc.color('reset') + ' ' + txt);
	}

	this._gprint = function(txt){ 
		print(this._cc.color('white','bgGreen', 'bold') + 'SUCCESS' + this._cc.color('reset') + ' ' + txt);
	}
	
	this._wprint = function(txt){ 
		print(this._cc.color('white','bgMagenta', 'bold') + 'WARN' + this._cc.color('reset') + ' ' + txt);
	}
	
	this._tagsMatch = function(tagp1, tagp2, approx) {
		if (tagp1.length != 3 || tagp2.length != 3) return false;
		if (isNaN(parseInt(tagp1[0])) || isNaN(parseInt(tagp1[1])) || isNaN(parseInt(tagp1[2]))) return false;
		if (isNaN(parseInt(tagp2[0])) || isNaN(parseInt(tagp2[1])) || isNaN(parseInt(tagp2[2]))) return false;
		return tagp1[0] == tagp2[0] && (approx ? (tagp1[1] == tagp2[1]) : true);
	}
	
	this._isTagCompatible = function(tag, tryTag) {
		var reqTagParts = tag.substring(1).split("."); //remove the first ~ or ^
		var tagParts = tryTag.substring(tryTag.charAt(0)=='v' ? 1 : 0).split(".");			
		if (tag.charAt(0)=='~' && this._tagsMatch(reqTagParts, tagParts, true)) {
			return tryTag;
		} else if (tag.charAt(0)=='^' && this._tagsMatch(reqTagParts, tagParts)) {
			return tryTag;
		} else if (tag==tryTag) {
			return tag;
		}
		return null;
	}
		
	this._resolveTag = function(proj, tag) {
		var tags = this._gitRequest(proj.tags_url);
		this._vprint("got tags for " + proj.name + ": " + tags.map(t => t.name).join(" "));
		//now match the requested tag...
		for (var i=0; i<tags.length; i++){
			var res = this._isTagCompatible(tag, tags[i].name);
			if (res) {
				return res;
			}
		}		
		return null;
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
	
	this._getPackageJson = function(pkgInstDir){ 
		var instPkgJsonPath = path.resolve(pkgInstDir, 'package.json');	
		try {
			var instPkgJson = JSON.parse(read(instPkgJsonPath));
			return instPkgJson;
		} catch (e) {
			return null;
		}			
	}
	
	this._getPackageVersion = function(pkgInstDir) { 
		var instPkgJson = this._getPackageJson(pkgInstDir);
		if (!instPkgJson) return null;
		if (typeof(instPkgJson.version)=='undefined') return null;
		return instPkgJson.version;	
	}
	
	this._getPackageDeps = function(pkgInstDir) { 
		var instPkgJson = this._getPackageJson(pkgInstDir);
		if (!instPkgJson) return null;
		if (typeof(instPkgJson.dependencies)=='undefined') return null;
		return instPkgJson.dependencies;	
	}	
	
	//git clone -b <tag_name> --single-branch <repo_url> [<dest_dir>] 
	this.install = function(depWhat, depVersion, depInstPath, ctx) {
		var tag = '';
		var pkg;
		var instDir;
		var obj;
		var isSave = false;
		var isRoot = false;
		if (!ctx) {
			ctx = {
				installed: [],
				failed: [],
				cwd: OS.cwd()
		    };		
			isRoot = true;
		}
		
		if (typeof(depWhat)=='undefined') {
			var isGlobal = this._stripFlag('global');
			isSave = true; //this._stripFlag('save');
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
			if (!obj) {
				print("\n");
				this._eprint("package installation failed.");
				return;
			}
			//print("tag=" + tag);
			
		} else {
			var searchArgs = depWhat.split(' ');
			obj = this._searchAndGetFirst.apply(this, searchArgs);		
			if (!obj) {
				ctx.failed.push({
					pkg: obj,
					tag: depVersion
				});				
				this._eprint("dependency not found: " + depWhat + (depVersion!='' ? '@'+depVersion : ''));
				return;
			}	
			tag = depVersion;
			instDir = depInstPath;
			pkg = searchArgs[0];
		}		
		
		if (!OS.isDirAndReadable(instDir)){					
			OS.mkdir(instDir);
			print("created install directory: " + instDir);
		}
		
		var pkgInstDir = path.resolve(instDir, pkg);
		if (OS.isDirAndReadable(pkgInstDir)) {
			var verifyFailMsg = null;
			
			var pkgVersion = this._getPackageVersion(pkgInstDir); 
			if (!pkgVersion || !this._isTagCompatible(tag, pkgVersion)) {
				verifyFailMsg = pkg + ' is already installed in ' + pkgInstDir + " but its version could not be verified to be compatible with the requested one."; 
			}
			
			if (verifyFailMsg) {
				ctx.failed.push({
					pkg: obj,
					tag: tag
				});				
				this._eprint(verifyFailMsg);
				this._eprint("You can try to remove this package if won't break other dependencies");
				if (isRoot) {
					print("\n");
					this._eprint("package installation failed.");				
				}
				return;				
			} else {
				print(pkg + ' is already installed in ' + pkgInstDir);
				return;
			}
		}		

		print("Installing " + obj.name + " in " + instDir);
		var git = this._getGit();
		var cmd = [git, 'clone'];
		if (tag!='') {
			var resolvedTag = this._resolveTag(obj, tag);
			if (!resolvedTag) {
				ctx.failed.push({
					pkg: obj,
					tag: tag
				});
				this._eprint("the requested tag " + tag + " was not found for " + obj.name);
				if (isRoot) {
					print("\n");
					this._eprint("package installation failed.");					
				}
				return;
			}
			tag = resolvedTag;
			cmd = cmd.concat(['-b', resolvedTag, '--single-branch']);
		}
		cmd.push(obj.git_url);
		cmd.push(pkgInstDir);
		this._vprint(cmd.join(' '));
		var res = OS.exec(cmd);
		
		if (OS.isDirAndReadable(pkgInstDir)) {
			//deps?
			var pkgJsonPath = path.resolve(pkgInstDir, 'package.json');
			
			//REMOVE ME
			if (!OS.isFileAndReadable(pkgJsonPath) && obj.name == 'tinn_web') {				
				var json = JSON.stringify({
					"dependencies": {
						"detect-newline": "^2.1.0",
						"uuid": "^3.3.2",
						"bluebird": "^3.5.3",
					}				
				},null,4);
				OS.writeFile(pkgJsonPath, json);
			}
			//END
			
			if (OS.isFileAndReadable(pkgJsonPath)) {
				var pkgJson = JSON.parse(OS.readFile(pkgJsonPath));
				if (typeof(pkgJson.dependencies)!='undefined') {
					this._vprint("handle " + obj.name + " deps..");
					for (var dep in pkgJson.dependencies) {
						print("Installing dependency: " + dep + ' ' + pkgJson.dependencies[dep]);
						this.install(dep, pkgJson.dependencies[dep], instDir, ctx);
					}
				}
			} else{
				print("no deps");
			}
			
			ctx.installed.push({
				pkg: obj,
				tag: tag,
				dir: pkgInstDir
			});
		} else {
			if (isRoot) {
				print('\n');
				this._eprint("package installation failed.");
				return;
			} else {
				ctx.failed.push({
					pkg: obj,
					tag: tag
				});
				this._eprint("failed to get "+ obj.name);				
				return;
			}
		}
		
		if (isRoot) {
			if (ctx.failed.length > 0) {
				print('one or more packages failed to install, rolling back...');
				for(var i=0; i<ctx.installed.length; i++){
					this._wprint("reverted installation of package " + ctx.installed[i].pkg.name);
					this._removeTree(ctx.installed[i].dir)					
				}
				print('\n');
				this._eprint("package installation failed.");
				return;
			} else {
				print('\n');
				this._gprint(pkg + ' ready.');
			}
		} else {
			this._gprint(pkg + ' installed');	
		}
		
		if (isRoot && ctx.installed.length > 0 && isSave) {
			var packageJsonPath = path.resolve(ctx.cwd, 'package.json');
			//print("updating package json " + packageJson);
			if (!OS.isFileAndReadable(packageJsonPath)) {
				this._wprint('package.json not found: ' + packageJsonPath);
			} else {
				try {
					var packageJson = JSON.parse(read(packageJsonPath));
					if (typeof(packageJson.dependencies)=='undefined') packageJson.dependencies = {};
					var pkgVersion = this._getPackageVersion(pkgInstDir); 
					if (!pkgVersion) pkgVersion = tag;
					packageJson.dependencies[pkg] = pkgVersion;
					OS.writeFile(packageJsonPath, JSON.stringify(packageJson, null, 4));
					print("updated package.json");
 
				} catch(e) {
					this._wprint('invalid content found in package.json: ' + packageJsonPath);					
				}
			}
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
	
	this.remove = function(pkg, pkgJson){
		var isGlobal = this._stripFlag('global');
		var isRoot = true;
		//var isSave = this._stripFlag('save');
		this._stripOtherFlags();
		
		if (typeof(pkg)!='undefined') {
			//recursive...
			isGlobal = false;
			isRoot = false;
		} else {
			if (this._args.length == 0) {
				print("no package given...");
				return;
			}				
			pkg = this._args[0];
		}
		
		var instDir = path.resolve(this._getInstalldir(isGlobal), 'tinn_modules');
		var pkgInstDir = path.resolve(instDir, pkg);
		if (!OS.isDirAndReadable(pkgInstDir)) {
			if (isRoot) {
				print("\n");
				this._eprint("Package " + pkg + " not found in " + instDir + ".");
				return;
			} else {
				this._wprint("Package " + pkg + " not found in " + instDir);
				return;
			}
		}
		
		//before removing, check package.json if there...
		var removePkgJsonPath = path.resolve(pkgInstDir, 'package.json');
		var removePkgJson = null;
		if (OS.isFileAndReadable(removePkgJsonPath)) {
			try {
				removePkgJson = JSON.parse(read(removePkgJsonPath));
			} catch(e) {
				this._wprint("failed to read " + pkg + " dependencies, uninstall may be incomplete");
			}
		}
		
		//before removing check if any other package needs it..
		var filesInInstDir = OS.listDir(instDir);
		if (filesInInstDir.length > 0) {
			for (var i=0;i<filesInInstDir.length; i++) {
				
				var deps = this._getPackageDeps(path.resolve(instDir, filesInInstDir[i]));
				if (deps && typeof(deps[pkg])!='undefined') {
					this._eprint("Cannot uninstall " + pkg + " because it is needed by " + filesInInstDir[i] +", uninstall " + filesInInstDir[i] + " first");
					if (isRoot) {
						print("\n");
						this._gprint("Uninstall failed.");						
					}
					return;
				}
			}			
		}
		
		print("Removing " + pkg + " from " + instDir);
		this._removeTree(pkgInstDir);
		
		if (!isGlobal) {
			var packageJsonPath = path.resolve(path.dirname(instDir), 'package.json');
			if (OS.isFileAndReadable(packageJsonPath) && removePkgJson){
				
				if (isRoot) {
					try {
						pkgJson = JSON.parse(read(packageJsonPath));				
					} catch(e) {
						this._wprint("failed to read local dependencies, uninstall may be incomplete");
					}
				}
				if (pkgJson) {
					for (var dep in removePkgJson.dependencies) {
						if (typeof(pkgJson.dependencies)=='undefined' || typeof(pkgJson.dependencies[dep])=='undefined') {
							this._vprint("uninstalling dependency " + dep);
							this.remove(dep, pkgJson);
						}
					}					
				}
			}
		}
		if (isRoot) {
			if (pkgJson) {
				if (typeof(pkgJson.dependencies)!='undefined') {
					delete pkgJson.dependencies[pkg];
					OS.writeFile(path.resolve(path.dirname(instDir), 'package.json'), JSON.stringify(pkgJson, null, 4));
					print("updated package.json");
				}
			}
			print("\n");
			this._gprint("Package uninstalled.");
		} else {
			this._gprint("Dependency " + pkg +" uninstalled");			
		}
	}
	
	this.uninstall = this.remove;
	
	this.search = function(what){ 
		var obj = this._searchAndGetAll(what);
		print("Found "  +obj.total_count + " projects: "); 
		for (var i=0; i<obj.items.length;i++) {
			var proj = obj.items[i];
			print(''+proj.name + " " + proj.id + " - " + proj.description);
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
		
	
	

	
