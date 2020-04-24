#!/usr/bin/env python3
'''
This works with the conjugation json file generated by
https://www.npmjs.com/package/french-verbs-lefff
'''
import mysql.connector
import json
import sys
import os

config = {
  'user': os.environ['USER'],
  'password': '',
  'unix_socket': '/var/lib/mysql/mysql.sock',
  'raise_on_warnings': True
}

TABLES = {}
TABLES['word'] = (
    'CREATE TABLE `words` ('
    '`word` VARCHAR(64) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,'
    '`data` BLOB,'
    'PRIMARY KEY(`word`)'
    ') ENGINE=InnoDB')
TABLES['invword'] = (
    'CREATE TABLE `invwords` ('
    '`invword` VARCHAR(64) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,'
    '`word` VARCHAR(64) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,'
    'KEY(`invword`), KEY(`word`)'
    ') ENGINE=InnoDB')

STATEMENTS = {'insert':{},'select':{}}
STATEMENTS['select']['word'] = (
    'SELECT 1 FROM `words` WHERE `word`=%(word)s LIMIT 1'
    )
STATEMENTS['insert']['word'] = (
    'INSERT INTO `words` (`word`,`data`) '
    "VALUES(%(word)s, %(data)s)"
    )
STATEMENTS['select']['invword'] = (
    'SELECT 1 FROM `invwords` '
    "WHERE `word`=%(word)s AND `invword`=%(invword)s LIMIT 1"
    )
STATEMENTS['insert']['invword'] = (
    'INSERT INTO `invwords` (`word`,`invword`) '
    'VALUES(%(word)s, %(invword)s)'
    )

def create_database(cursor):
  try:
    cursor.execute('USE french')
    print(f'Connected to database.')
  except mysql.connector.Error as err:
    if err.errno != mysql.connector.errorcode.ER_BAD_DB_ERROR:
      raise
    cursor.execute("CREATE DATABASE french DEFAULT CHARACTER SET 'utf8'")
    print(f'Created database.')
    cursor.execute('USE french')

def create_tables(cursor):
  for t,c in TABLES.items():
    try:
      cursor.execute(c)
      print(f'Created table {t}.')
    except mysql.connector.Error as err:
      if err.errno != mysql.connector.errorcode.ER_TABLE_EXISTS_ERROR:
        raise
      print(f'Table {t} already exists.')

def convert(conj_file):
  conn = mysql.connector.connection.MySQLConnection(**config)
  conn.set_charset_collation('utf8', 'utf8_bin')
  cursor = conn.cursor()
  create_database(cursor)
  create_tables(cursor)
  datafile = open(conj_file, 'r')
  data = json.load(datafile)
  for word, forms in data.items():
    cursor.execute(STATEMENTS['select']['word'], {'word':word})
    dat = cursor.fetchall()
    if dat:
      raise f'{word} already exists in database?'
    print(f'inserting {word}')
    cursor.execute(STATEMENTS['insert']['word'],
        {'word':word, 'data':json.dumps(forms)})
    for k,v in forms.items():
      if not v:
        continue
      for invword in v:
        if invword == 'NA' or not invword:
          continue
        cursor.execute(STATEMENTS['select']['invword'],
            {'word':word, 'invword':invword})
        dat = cursor.fetchall()
        if not dat:
          cursor.execute(STATEMENTS['insert']['invword'],
              {'word':word, 'invword':invword})
    conn.commit()
  cursor.close()
  conn.close()

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print(f'Usage: {sys.argv[0]} CONJUGATION_FILE')
    sys.exit(1)
  convert(sys.argv[1])
