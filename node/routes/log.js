var express = require('express');
var router = express.Router();
var fs = require('fs');
var logfile = 'holchain.log';
var listlen = 4096;

log = function(logdata, cb = null) {
  now = Date.now() / 1000;
  var logdata = '[' + now + '] ' + logdata;
  fs.appendFile(logfile, logdata + "\n", (err) => {
    if (cb !== null) cb(err, logdata);
  });
};

router.get('/', function(req, res, next) {
  log(req.query.log, (err, logdata) => {
    if (err) throw err;
    res.send(logdata);
  });
});

router.get('/list', function(req, res, next) {
  fs.open(logfile, 'r', (err, fd) => {
    if (err) throw err;
    fs.stat(logfile, (err, stats) => {
      if (err) throw err;
      var size = stats.size;
      var buf = new Buffer(listlen);
      fs.read(fd, buf, 0, listlen, size - listlen, (err, br, buf) => {
        res.send(buf);
      });
    });
  });
});

module.exports = router;


