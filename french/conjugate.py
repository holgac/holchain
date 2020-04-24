#!/usr/bin/env python3
import mysql.connector
import json
import sys
import os

config = {
  'user': os.environ['USER'],
  'password': '',
  'database': 'french',
  'unix_socket': '/var/lib/mysql/mysql.sock',
  'raise_on_warnings': True
}

STATEMENTS = {}
STATEMENTS['select'] = (
    'SELECT w.data '
    'FROM words w INNER JOIN invwords i '
    'ON (i.word=w.word AND i.invword=%(word)s)'
    )
HTML = {}
HTML['prefix']='''<html>
  <head>
    <style>
      table, th, td {
        border: 1px solid black;
        border-collapse: collapse;
        padding: 3px;
      }
      .word {
        color: red;
        font-weight: 900;
      }
      .tense {
        color: green;
        font-weight: 500;
      }
    </style>
  </head>
  <body>
'''
HTML['suffix']='</body></html>'

def table_header(word):
  result = f'<tr><th class="word">{word}</th>'
  for subj in ['je', 'tu', 'il/elle', 'nous', 'vous', 'ils/elles']:
    result += f'<th>{subj}</th>'
  result += '</tr>'
  return result

def table_row(tense, data):
  result = f'<tr><td class="tense">{tense}</td>'
  for d in data:
    result += f'<td>{d}</td>'
  result += '</tr>'
  return result

def conjugate(word):
  conn = mysql.connector.connection.MySQLConnection(**config)
  conn.set_charset_collation('utf8', 'utf8_bin')
  cursor = conn.cursor()
  cursor.execute(STATEMENTS['select'], {'word':word})
  res = cursor.fetchall()
  if not res:
      return None
  result = HTML['prefix']
  for row in res:
    data = json.loads(row[0])
    result += '<table>' + table_header(data['W'][0])
    result += table_row('présent indicatif', data['P'])
    result += table_row('imparfait indicatif', data['I'])
    result += table_row('futur', data['F'])
    result += table_row('passé simple', data['J'])
    result += table_row('présent conditionnel', data['C'])
    result += table_row('présent subjonctif', data['S'])
    result += table_row('imparfait subjonctif', data['T'])
    result += table_row('présent imperatif', data['Y'])
    result += table_row('présent participe', data['G'])
    result += table_row('passé participe', data['K'])
    result += '</table>'
  result += HTML['suffix']
  return result

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print(f'Usage: {sys.argv[0]} WORD')
    sys.exit(1)
  result = conjugate(sys.argv[1])
  if result:
      print(result)
