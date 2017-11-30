var express = require('express');
var router = express.Router();
var flatfile = require('flat-file-db');
var db = flatfile('localdb.db');

router.get('/', function(req, res, next) {
  var map = db.get('map');
  var key = req.query.key;
  var value = map[key];
  if (value === undefined) {
    res.status(404).send('not found');
  } else {
    res.send(value);
  }
});

router.get('/add', function(req, res, next) {
  var map = db.get('map');
  if (map === undefined) {
    map = {};
  }
  var key = req.query.key;
  var value = req.query.value;
  map[key] = value;
  db.put('map', map);
  res.send(value);
});

router.get('/list', function(req, res, next) {
  var map = db.get('map');
  res.send(JSON.stringify(map));
});

module.exports = router;

