var express = require('express');
var router = express.Router();
var flatfile = require('flat-file-db');
var db = flatfile('redirect.db');

function search(query, idx, node) {
  if (query.length > idx) {
    var subq = query[idx];
    if (node.urlmap[subq]) {
      return search(query, idx+1, node.urlmap[subq]);
    }
  }
  return {node:node, idx:idx};
}

function geturl(query, urlmap) {
  // TODO; multipe arg support
  var res = search(query, 0, urlmap);
  return res.node['default'].replace(
    /%s/g,
    query.slice(res.idx).join(' '),
  );
}

/* redirect based on the query */
router.get('/', function(req, res, next) {
  var query = req.query.q.split(' ');
  var urlmap = db.get('urlmap');
  res.redirect(302, geturl(query, urlmap));
});

router.get('/add', function(req, res, next) {
  var query = req.query.q.split(' ');
  var urlmap = db.get('urlmap');
  var result = search(query, 0, urlmap)
  if (result.idx == query.length - 2) {
    result.node.urlmap[query[result.idx]] = {
      "default":query[result.idx+1],
      urlmap:{}
    };
    db.put('urlmap', urlmap);
  } else if (result.idx == query.length - 1) {
    result.node['default'] = query[result.idx];
    db.put('urlmap', urlmap);
  } else {
    // TODO: remove if after implementing error handling
    undefined.undefined = 5;
  }
  res.send(JSON.stringify(urlmap));
});

router.get('/list', function(req, res, next) {
  var urlmap = db.get('urlmap');
  res.send(JSON.stringify(urlmap, null, 2));
});

module.exports = router;
