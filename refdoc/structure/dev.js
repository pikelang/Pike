/*

  Encapsulate so we don't clutter the global scope

*/
(function(window, document) {
'use strict';

var sticky_scroll_break = 70;
var navbar, content, navbar_height, jx;

// Hide the navbox when on the start page where the prev and next links
// doesn't lead anywhere
var maybe_hide_navbox = function() {
  var navbox = document.getElementsByClassName('navbox')[0];
  var prev = navbox.getElementsByClassName('prev')[0];
  var next = navbox.getElementsByClassName('next')[0];
  prev = prev.getAttribute('href');
  next = next.getAttribute('href');

  if (!next && !prev) {
    navbox.style.display = 'none';
    hide_top_link();
  }
};

// And if on the start page hide the Top link in the side navbar
var hide_top_link = function() {
  var top = document.getElementsByClassName('top head');
  top[0].style.display = 'none';
};

var swap_scripts = function(new_html) {
  var head = document.head;
  var new_head = new_html.head;
  var old_scripts = head.getElementsByTagName('script');
  var new_scripts = new_head.getElementsByTagName('script');
  var i, s, src;

  var os = [], ns = [];

  for (i = 0; i < old_scripts.length; i++)
    os.push(old_scripts[i]);

  for (i = 0; i < new_scripts.length; i++)
    ns.push(new_scripts[i]);

  for (i = 0; i < os.length; i++) {
    s = os[i];
    src = s.getAttribute('src');

    if (src && src.split('/').slice(-1)[0] === 'site.js') {
      continue;
    }

    s.parentNode.removeChild(s);
  }

  console.log('ns: ', ns);

  for (i = 0; i < ns.length; i++) {
    s = ns[i];
    src = s.getAttribute('src');

    if (src && src.split('/').slice(-1)[0] === 'site.js') {
      continue;
    }

    var ss = document.createElement('script');

    if (src) {
      ss.src = src;
    }
    else {
      ss.appendChild(document.createTextNode(s.innerText));
    }

    head.appendChild(ss);
  }

  window.navbar(document);
}

var swap_page = function(url) {
  var page = document.getElementById('page');
  var head = document.head;

  jx.get(url, function(data) {
    var html = new DOMParser().parseFromString(data, 'text/html');
    var new_page = html.getElementById('page');

    push_state({
      from: {
        title: document.title,
        location: document.location.href
      },
      to: {
        title: html.title,
        location: url
      }
    }, html.title, url);

    swap_scripts(html);
    page.parentNode.replaceChild(new_page, page);
    on_page_load(new_page);
  });
};

var push_state = function(obj, title, url) {
  if (window.history && window.history.pushState) {
    window.history.pushState(obj, title, url);
  }
};

var on_page_load = function(base) {
  maybe_hide_navbox();
  navbar = document.getElementsByClassName('navbar')[0];
  content = document.getElementsByClassName('content')[0];
  navbar_height = window.outerHeight - content.offsetTop -
                  (document.getElementsByTagName('footer')[0].offsetHeight);

  if (document.location.host && window.DOMParser) {
    var elems = (base||document).getElementsByTagName('a');
    for (var i = 0; i < elems.length; i++) {
      var el = elems[i],
          href = el.getAttribute('href'),
          lchref = undefined;

      if (!href) continue;

      lchref = href.toLowerCase();

      if (href.substring(0, 'http://'.length)  !== 'http://'  &&
          href.substring(0, 'https://'.length) !== 'https://' &&
          href.substring(0, 'ftp://'.length)   !== 'ftp://'   &&
          href[0] !== '/')
      {
        el.addEventListener('click', function(ev) {
          ev.preventDefault();
          swap_page(this.getAttribute('href'));
          return false;
        }, false);
      }
    };
  }
};

// We only care about fairly modern browsers
if (document.addEventListener) {
  // Fire when the DOM is ready
  document.addEventListener("DOMContentLoaded", function(e) {
    on_page_load();
  });

  document.addEventListener('scroll', function(e) {
    if (window.scrollY > sticky_scroll_break) {
      if (window.i_am_sticky) {
        return;
      }
      window.i_am_sticky = true;
      content.style.minHeight = navbar_height + 'px';
      navbar.classList.add('sticky');
    }
    else {
      if (window.i_am_sticky === false) {
        return;
      }
      window.i_am_sticky = false;
      navbar.classList.remove('sticky');
      content.style.minHeight = 0;
    }
  })
}

jx = (new function() {
  var __jx = function(method, url, data, callback) {
    var req;

    var bind_function = function(fun, obj) {
      return function() { fun.apply(obj, [ obj ]); };
    };

    var on_state_change = function(obj) {
      if (req.readyState === 4) {
        callback(req.responseText);
      }
    };

    var req_create = function() {
      if (window.ActiveXObject) {
        return new ActiveXObject('Microsoft.XMLHTTP');
      }
      else if (window.XMLHttpRequest) {
        return new XMLHttpRequest();
      }
      return false;
    };

    if (req = req_create()) {
      req.onreadystatechange = bind_function(on_state_change, this);
      req.open("GET", url, true);
      try { req.send(data); }
      catch (e) { console.log('Error: ', e); }
    }
  };

  var obj_to_query = function(obj) {
    var s = [];
    if (obj) {
      if (typeof obj === 'object') {
        Object.keys(obj).forEach(function(k) {
          s.push(k + '=' + encodeURIComponent(obj[k]));
        });
      }

      return s.join('&');
    }
  };

  return {
    get: function(url, vars, callback) {
      if (typeof vars === 'function') {
        callback = vars;
        vars = null;
      }
      return new __jx('GET', url, obj_to_query(vars), callback);
    }
  };
}());

}(window, document));