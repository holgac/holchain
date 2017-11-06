var express = require('express');
var router = express.Router();

/* redirect based on the query */
router.get('/', function(req, res, next) {
  // var query = req.query.q;
  res.redirect(302, 'http://www.example.com');
});

module.exports = router;
