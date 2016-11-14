/* jshint undef: true, unused: true */
/* globals window, document, requestAnimationFrame */
/* exported PikeDoc */

var PikeDoc = null;

if (!window.console) {
  window.console = { log: function(){}, error: function(){} };
}

/*
  Encapsulate so we don't clutter the global scope
*/
(function(window, document) {
'use strict';

var doDebug = false;
var isdebug = document.location.search.indexOf('debug=1') > -1 || doDebug;
var wdebug  = isdebug ? window.console.log : function(){};

// The scroll position at which the navbar sticks. This actually gets
// calculated dynamically upon page load.
var stickyScrollBreak = 70,
    // Window width when we go to mobile mode
    mobileBreakPoint = 800,
    // The navbar to the left, HTMLElement
    navbar, innerNavbar,
    // Content wrapper, HTMLElement
    content,
    // The page footer
    footer,
    // The height of the navbar
    navbarHeight,
    // The height of the window
    windowHeight,
    // The height of the header
    headerHeight,
    // The height of the footer
    footerHeight,
    // The menu hamburger when in mobile mode
    burger,
    // Optimization®
    objectKeys = Object.keys,
    // Functions to run when DOM is ready
    onDOMReadyQueue = [];

// Hide the navbox when on the start page where the prev and next links
// doesn't lead anywhere
function maybeHideNavbox() {
  var navbox = document.getElementsByClassName('navbox')[0];
  var prev   = navbox.getElementsByClassName('prev')[0];
  var next   = navbox.getElementsByClassName('next')[0];
  prev       = prev.getAttribute('href');
  next       = next.getAttribute('href');

  if (!next && !prev) {
    navbox.style.display = 'none';
    hideTopLink();
  }
}

// And if on the start page hide the Top link in the side navbar
function hideTopLink() {
  var top = document.getElementsByClassName('top head');
  top[0].style.display = 'none';
}

// Called when DOM is ready
function onPageLoad() {
  var versionElems, dateElems, i, max;

  maybeHideNavbox();
  navbar       = document.getElementsByClassName('navbar')[0];
  content      = document.getElementsByClassName('content')[0];
  footer       = document.getElementsByTagName('footer')[0];
  footerHeight = footer.offsetHeight;
  windowHeight = window.outerHeight;
  headerHeight = document.getElementsByTagName('header')[0].offsetHeight;
  navbarHeight = windowHeight - content.offsetTop - footerHeight;
  innerNavbar  = document.getElementById('navbar');
  burger       = document.getElementById('burger');

  // When the doc is compiled with FLAG_NO_DYNAMIC the version and publish date
  // will not be written to the pages but inserted with JS. If the NO_DYNAMIC
  // symbol is true we need to put the version and pubdate in the elements with
  // attributes data-id="version" and data-id="date".
  if (PikeDoc.NO_DYNAMIC) {
    versionElems = document.querySelectorAll('[data-id="version"]');
    dateElems = document.querySelectorAll('[data-id="date"]');
    max = Math.max(versionElems.length, dateElems.length);

    for (i = 0; i < max; i++) {
      if (versionElems[i] !== undefined) {
        versionElems[i].innerHTML = PikeDoc.VERSION;
      }
      if (dateElems[i] !== undefined) {
        dateElems[i].innerHTML = PikeDoc.PUBDATE;
      }
    }
  }

  stickyScrollBreak = headerHeight;
}

var iAmSticky;
// Invoked when DOM is ready, and use as callback for onscroll.
function onPageScroll() {
  // If scrollY is larger than the sticky position ...
  if (window.scrollY > stickyScrollBreak) {
    // ... see if we're already sticky and return if so ...
    if (iAmSticky) {
      return;
    }
    // ... or else set to sticky.
    iAmSticky = true;
    content.style.minHeight = (windowHeight - headerHeight) + 'px';
    navbar.classList.add('sticky');
  }
  // If scrollY is less than the sticky position ...
  else {
    // ... see if we're explicitly non-sticky and return if so ...
    if (iAmSticky === false) {
      return;
    }
    // ... else set to explicitly non-sticky
    iAmSticky = false;
    navbar.classList.remove('sticky');
    content.style.minHeight = 0;
  }
}

var iAmScrolled;
function onMobilePageScroll() {
  if (window.scrollY > 1) {
    if (iAmScrolled) {
      return;
    }
    iAmScrolled = true;
    document.body.classList.add('scrolled');
  }
  else {
    if (iAmScrolled === false) {
      return;
    }
    iAmScrolled = false;
    document.body.classList.remove('scrolled');
  }
}

function onBurgerClick() {
  document.body.classList.toggle('menu-open');
  return false;
}

function setMobileMode() {
  document.removeEventListener('scroll', onPageScroll);
  document.addEventListener('scroll', onMobilePageScroll, false);
  burger.removeEventListener('click', onBurgerClick);
  burger.addEventListener('click', onBurgerClick, false);
  navbar.classList.remove('sticky');
  iAmSticky = false;
}

function setDesktopMode() {
  document.removeEventListener('scroll', onMobilePageScroll);
  document.addEventListener('scroll', onPageScroll, false);
  burger.removeEventListener('click', onBurgerClick);
  document.body.classList.remove('menu-open');
}

var iAmMobile = false;
function onWindowResize() {
  if (document.body.offsetWidth < mobileBreakPoint) {
    if (iAmMobile) {
      return;
    }
    iAmMobile = true;
    document.body.classList.add('mobile');
    setMobileMode();
  }
  else {
    if (iAmMobile === false) {
      return;
    }
    iAmMobile = false;
    document.body.classList.remove('mobile');
    setDesktopMode();
  }
}

// We only care about fairly modern browsers
if (document.addEventListener) {
  // Fire when the DOM is ready
  document.addEventListener('DOMContentLoaded', function() {
    onPageLoad();
    cacheFactory.setMenu();
    PikeDoc.domReady(true);
    onWindowResize();
    window.addEventListener('resize', onWindowResize, false);
    document.addEventListener('scroll', iAmMobile ? onMobilePageScroll
                                                  : onPageScroll,
                                        false);
  }, false);
}

// During a session each generated menu is cached locally in a sessionStorage
// (if available). This one handles that.
var cacheFactory = (function() {
  // Don't use cache if the page isn't served through a server.
  // The cache seems buggy as hell when the pages are view directly from
  // the file system.
  var cache = document.location.hostname && window.sessionStorage;
  var m, isChecked = false;

  function init() {
    if (m || PikeDoc.current.link === 'index.html') {
      return true;
    }

    if (!cache || (isChecked && !m)) {
      return false;
    }

    m = cache.getItem(PikeDoc.current.link);
    isChecked = true;

    if (m) {
      m = JSON.parse(m);
      var ok = validateDate(m.time);
      if (!ok) {
        isChecked = false;
        cache.removeItem(PikeDoc.current.link);
      }

      return ok;
    }

    return false;
  }

  function validateDate(time) {
    return getPubDate() < new Date(time);
  }

  function getPubDate() {
    return new Date(PikeDoc.GENERATED*1000);
  }

  function store() {
    if (cache) {
      var obj = {
	      time: Date.now(),
        value: innerNavbar.innerHTML
      };

      cache.setItem(PikeDoc.current.link||'root', JSON.stringify(obj));
    }
  }

  function setMenu() {
    if (m && validateDate(m.time)) {
      //window.console.log('Set menu');
      innerNavbar.innerHTML = m.value;
      requestAnimationFrame(function() {
        innerNavbar.querySelector('.sidebar').classList.remove('init');
      });
    }
  }

  return {
    hasCache: init,
    store: store,
    setMenu: setMenu
  };
}());

// Create a document element
//
// @param string name
//  Tag name
// @param object|string attr
//  If a string treated as a text node, otherwise as tag attributes
// @param string text
function createElem(name, attr, text) {
  var e = document.createElement(name);
  if (attr && typeof attr === 'object') {
    objectKeys(attr).forEach(function(k) {
      e.setAttribute(k, attr[k]);
    });
  }
  else if (typeof attr === 'string') {
    e.appendChild(document.createTextNode(attr));
  }

  if (text) {
    e.appendChild(document.createTextNode(text));
  }

  return e;
}

var helpers = (function() {
  // Returns basedir of `path`
  function basedir(path) {
    var i = path.lastIndexOf('/');
    if (i < 1) return '';
    return path.substring(0, i);
  }

  // Check if `other` starts with `prefix`
  function hasPrefix(prefix, other) {
    return other.substring(0, prefix.length) === prefix;
  }

  function adjustLink(link) {
    var reldir = basedir(PikeDoc.current.link);
    var dots = '';
    while (reldir !== '' && !hasPrefix(link, reldir + '/')) {
      dots += '../';
      reldir = basedir(reldir);
    }
    return dots + link.substring(reldir.length);
  }

  // Merge two sets of nodes. If a node exists in both `oldNodes` and
  // `newNodes` the latter will overwrite the former.
  function mergeChildren(oldNodes, newNodes) {
    var hash = {};
    oldNodes.forEach(function(n) {
      hash[n.name] = n;
    });

    newNodes.forEach(function(n) {
      var j = hash[n.name];
      if (j) j = n;
      else hash[n.name] = n;
    });

    return objectKeys(hash).map(function(k) { return hash[k]; })
                           .sort(function(a, b) {
                             return (a.name > b.name) - (b.name > a.name);
                           });
  }

  return {
    basedir:       basedir,
    hasPrefix:     hasPrefix,
    adjustLink:    adjustLink,
    mergeChildren: mergeChildren
  };
}());

// Main object for generating the navigation
PikeDoc = (function() {
  var symbols     = [],
      symbolsMap  = {},
      endInherits = [],
      inheritList = [],
      isDomReady  = false,
      isAllLoaded = false,
      isInline    = true,
      current;

  function Symbol(name) {
    this.name = name;
    this._children = {};
    this.children = [];
  }

  Symbol.prototype = {
    addChildren: function(type, children) {
      this._children[type] = children;
      return this;
    },
    finish: function() {
      var my = this;

      // window.console.log('### Symbol.finish(', this.name, ')');

      objectKeys(this._children).forEach(function(k) {
        my.children = my.children.concat(my._children[k]);
      });

      lowSetInherit();
    },
    setInherited: function() {
      this.children.forEach(function(c) {
        c.inherited = 1;
      });
    }
  };

  function lowSetInherit() {
    endInherits = endInherits.filter(function(a) {
      var ss = symbolsMap[a];
      if (ss) {
        ss.setInherited();
        return false;
      }

      return true;
    });
  }

  /* This is called from the generated javascripts (index.js) that's being
   * loaded on the fly.
   *
   * @param string name
   *  The name of the symbol (Namespace, module e.t.c)
   * @param boolean isInline
   *  Is this being called from a script loaded directly in the page
   *  or being called from a loaded script.
   */
  function registerSymbol(name, isInline) {
    // Only the parent namespace/module/class is loaded inline, and we don't
    // care about that one. Also, when on the TOP page the navigation is
    // written to the page, so we don't care for that either.
    if (isInline || !name) {
      return new Symbol(name);
    }
    var s = new Symbol(name);
    symbols.push(s);
    //window.console.log('   + Register symbol: ', name);
    symbolsMap[name] = s;
    return s;
  }

  function endInherit(which) { endInherits.push(which); }
  function addInherit(which) { inheritList = inheritList.concat(which); }

  var types = {};
  function finish() {
    // window.console.log('finish(', endInherits.length, ')');
    if (endInherits.length === 0) {
      var merge = helpers.mergeChildren;
      objectKeys(symbolsMap).forEach(function(k) {
        var ch = symbolsMap[k]._children;
        objectKeys(ch).forEach(function(sk) {
          types[sk] = merge(types[sk]||[], ch[sk]);
        });
      });

      isAllLoaded = true;
      maybeRenderNavbar();
    }
  }

  var jsMap = {};
  var scriptQueue = 0;

  function loadScript(link, namespace, inherits) {
    wdebug('load: ', link);
    if (cacheFactory.hasCache()) {
      return;
    }

    link = helpers.adjustLink(link);

    // Already loaded
    if (jsMap[link]) {
      wdebug('Already loaded: ', link);
      return;
    }

    wdebug('+++ Load:', link);

    jsMap[link] = true;

    if (inherits) {
      addInherit(inherits);
    }

    scriptQueue += 1;

    var s = createElem('script', { src: link });
    //s.async = false;
    document.head.appendChild(s);

    (function(scr, ns) {
      scr.addEventListener('load', function() {
        scriptQueue -= 1;

        if (ns) {
          if (ns === true) { finish(); }
          else { endInherit(ns); }
        }
        else {
          finish();
        }
      }, false);
    }(s, namespace));
  }

  function domReady() {
    isDomReady = true;
    onDOMReadyQueue.forEach(function(f) {
      if (typeof f === 'function') {
        f();
      }
    });
    maybeRenderNavbar();
  }

  function lowNavbar(container, heading, nodes, suffix) {
    if (!nodes || !nodes.length) {
      return;
    }

    var curlnk = PikeDoc.current.link;
    var adjlnk = helpers.adjustLink;
    var c      = container;
    var div    = createElem('div', { style: 'margin-left:0.5em' });

    nodes.forEach(function(n) {
      var name, tnode, tmp;
      name = n.name + suffix;
      tnode = document.createTextNode(name);

      if (!n.inherited) {
        tnode = createElem('b', name);
      }

      if (n.link !== curlnk) {
        tmp = createElem('a', { href: adjlnk(n.link) });
        tmp.appendChild(tnode);
        tnode = tmp;
      }

      if (n.modifiers) {
        n.modifiers.forEach(function(mod) {
          if (n.name !== 'create') {
            tnode.classList.add('mod-' + mod);
          }
        });
      }

      div.appendChild(tnode);
    });

    c.appendChild(createElem('b', { class: 'heading' }, heading));
    c.appendChild(div);
  }

  /* Render the left navigation bar. */
  function navbar() {
    var s = createElem('div', { class: 'sidebar init' });
    // If the cache already has set the menu, then clear it. The cache is
    // almost certainly run before this method.
    var old = innerNavbar.querySelectorAll('.sidebar');
    var i, tmp;
    if (old.length) {
      wdebug('Clear cached menu and regenerate', old);
      for (i = 0; i < old.length; i++) {
        tmp = old[i];
        tmp.parentNode.removeChild(tmp);
      }
    }

    innerNavbar.appendChild(s);

    lowNavbar(s, 'Modules',    types.module,    '');
    lowNavbar(s, 'Classes',    types.class,     '');
    lowNavbar(s, 'Enums',      types.enum,      '');
    lowNavbar(s, 'Directives', types.directive, '');
    lowNavbar(s, 'Methods',    types.method,    '()');
    lowNavbar(s, 'Operators',  types.operator,  '()');
    lowNavbar(s, 'Members',    types.member,    '()');
    lowNavbar(s, 'Namespaces', types.namespace, '::');
    lowNavbar(s, 'Appendices', types.appendix,  '');

    cacheFactory.store();
  }

  function maybeRenderNavbar() {
    wdebug('maybeRenderNavbar(', isAllLoaded, isDomReady, scriptQueue, ')');
    if (isAllLoaded && isDomReady && scriptQueue === 0) {
      navbar();
      requestAnimationFrame(function() {
        innerNavbar.querySelector('.sidebar').classList.remove('init');
      });
    }
  }

  (function() {
    // If the refdoc lives in pike.lysator.liu.se we add some custom Google
    // searchability.
    if (document.location.hostname === 'pike.lysator.liu.se' ||
        document.location.hostname === 'pike.local') // for dev purposes
    {
      onDOMReadyQueue.push(function() {
        // When this is run on pike.lysator.liu.se the script below will replace
        // the content of #version with a search field. Since the script below
        // is loaded async there might be the case where the version is
        // briefly shown before it's replaced, which will produce an unpleasant
        // flicker. This hack will minimize that "unpleasantry".
        var v = document.getElementById('version');
        if (!v.classList.contains('search')) {
          v.innerHTML = '';
        }
      });

      var s  = document.getElementsByTagName('script')[0];
      var el = createElem('script', {
        src: '/assets/js/local/refdoc-search.min.js',
        async: true
      });

      s.parentNode.insertBefore(el, s);

      var el2 = createElem('script', {
        src: '/assets/js/local/disqus.min.js',
        async: true
      });

      s.parentNode.insertBefore(el2, s);

      var f = createElem('link', {
        href: '/assets/img/favicon.png',
        rel: 'shortcut icon'
      });

      document.head.appendChild(f);
    }
  }());

  return {
    registerSymbol: registerSymbol,
    endInherit:     endInherit,
    loadScript:     loadScript,
    domReady:       domReady,
    isInline:       isInline,
    current:        current,
    finish:         finish
  };
}());

// This is explicitly set to false in the HTML page if the docs are generated
// with inlined Pike version and timestamp.
PikeDoc.FLAG_NO_DYNAMIC = true;

}(window, document));
