#!/usr/bin/env python3

import json
import os
import argparse
import enum
import sys
import re

class SupplementList:
  def __init__(self, name, supplements, notes):
    self.name = name
    self.supplements = supplements
    self.notes = notes
  @staticmethod
  def load(obj):
    return SupplementList(
      obj['name'],
      obj['supplements'],
      obj['notes'],
    )
  @staticmethod
  def populate_argparse(p, required):
    p.add_argument('--name', '-n', type=str, dest='name', required=required)
    p.add_argument('--supplements', '-s', type=str, dest='supplements', nargs='+', required=required)
    p.add_argument('--notes', '-e', type=str, dest='notes', default=[])
  def sanitize(self):
    pass
  def validate(self, db):
    for s in self.supplements:
      if db.get_supplement(s) is None:
        raise ValueError(f'{s} not a valid supplement')
  def serialize(self, fields=None):
    self.sanitize()
    ser = self.__dict__
    if fields:
      ser = {k:ser[k] for k in fields}
    return ser

class Supplement:
  def __init__(self, name, dosew, dosev, inc, noop, dec, time, notes, exps):
    self.name = name
    # 1 ounce = 1/16 pounds = 28 grams
    # weight in grams
    self.dosew = dosew
    # 1tbsp = 3tsp = 0.5 fl.oz = 5ml
    # volume in milliliters
    self.dosev = dosev
    self.inc = inc
    self.noop = noop
    self.dec = dec
    # TODO: enum
    self.time = time
    self.notes = notes
    self.exps = exps
    self.sanitize()
  def validate(self, db):
    pass
  @staticmethod
  def populate_argparse(p, required):
    p.add_argument('--name', '-n', type=str, dest='name', required=required)
    p.add_argument('--dosew', '-w', type=float, dest='dosew', required=required)
    p.add_argument('--dosev', '-v', type=float, dest='dosev', required=required)
    p.add_argument('--inc', '-i', type=str, dest='inc', nargs='*', default=[])
    p.add_argument('--noop', '-o', type=str, dest='noop', nargs='*', default=[])
    p.add_argument('--dec', '-d', type=str, dest='dec', nargs='*', default=[])
    p.add_argument('--time', '-t', type=str, dest='time', nargs='*', default='any')
    p.add_argument('--notes', '-e', type=str, dest='notes', nargs='*', default=[])
    p.add_argument('--exps', '-x', type=str, dest='exps', nargs='*', default=[])
  @staticmethod
  def load(obj):
    return Supplement(
      obj['name'],
      obj['dosew'],
      obj['dosev'],
      obj['inc'],
      obj['noop'],
      obj['dec'],
      obj['time'],
      obj['notes'],
      obj['exps'],
    )
  def sanitize(self):
    pass
  def serialize(self, fields=None):
    self.sanitize()
    ser = self.__dict__
    if fields:
      ser = {k:ser[k] for k in fields}
    return ser

class Database:
  def __init__(self, filepath):
    self.supplements = []
    self.supplement_lists = []
    self.filepath = filepath
    self.actions = {
      Supplement.__name__: {
        'get': self.get_supplement,
        'add': self.add_supplement,
        'all': self.supplements,
      },
      SupplementList.__name__: {
        'get': self.get_supplement_list,
        'add': self.add_supplement_list,
        'all': self.supplement_lists,
      }
    }
    self._load()
  def _load(self):
    try:
      dat = json.load(open(self.filepath, 'r'))
    except FileNotFoundError:
      dat = {}
    if 'supplements' not in dat:
      dat['supplements'] = []
    if 'supplement_lists' not in dat:
      dat['supplement_lists'] = []
    for s in dat['supplements']:
      self.add_supplement(Supplement.load(s))
    for s in dat['supplement_lists']:
      self.add_supplement_list(SupplementList.load(s))
  def add_item(self, cls, val):
    val.validate(self)
    if cls.__name__ not in self.actions:
      raise ValueError(f'{cls.__name__} not implemented')
    return self.actions[cls.__name__]['add'](val)
  def add_supplement_list(self, supp_list):
    self.supplement_lists.append(supp_list)
  def add_supplement(self, supp):
    self.supplements.append(supp)
  def get_items(self, cls):
    if cls.__name__ not in self.actions:
      raise ValueError(f'{cls.__name__} not implemented')
    return self.actions[cls.__name__]['all']
  def get_item(self, cls, name):
    if cls.__name__ not in self.actions:
      raise ValueError(f'{cls.__name__} not implemented')
    return self.actions[cls.__name__]['get'](name)
  def get_supplement_list(self, name):
    for s in self.supplement_lists:
      if s.name == name:
        return s
    return None
  def get_supplement(self, name):
    for s in self.supplements:
      if s.name == name:
        return s
    return None
  def dump(self):
    json.dump(
      {
        'supplements':
          [i for i in map(lambda s: s.serialize(), self.supplements)],
        'supplement_lists':
          [i for i in map(lambda s: s.serialize(), self.supplement_lists)],
      }, 
      open(self.filepath, 'w'),
      indent=2,
      sort_keys=True,
    )

class Action(enum.Enum):
  print_supplement = 'print_supplement'
  print_list = 'print_list'
  print_supplements = 'print_supplements'
  print_lists = 'print_lists'
  add_supplement = 'add_supplement'
  add_list = 'add_list'
  edit_supplement = 'edit_supplement'
  def __str__(self):
    return self.value

class Main:
  def __init__(self):
    self.args = None
    self.actions = {
      str(Action.print_supplement): lambda: self.print_item(Supplement),
      'p': lambda: self.print_item(Supplement),
      str(Action.print_list): lambda: self.print_item(SupplementList),
      'pl': lambda: self.print_item(SupplementList),
      str(Action.print_supplements): lambda: self.print_items(Supplement),
      'pa': lambda: self.print_items(Supplement),
      str(Action.print_lists): lambda: self.print_items(SupplementList),
      'pal': lambda: self.print_items(SupplementList),
      str(Action.add_supplement):  lambda: self.add_item(Supplement),
      'a':  lambda: self.add_item(Supplement),
      str(Action.add_list):  lambda: self.add_item(SupplementList),
      'al':  lambda: self.add_item(SupplementList),
    }
    self.ap = argparse.ArgumentParser()
    sub = self.ap.add_subparsers(help='Action', title='actions', dest='action')
    p = sub.add_parser(
        str(Action.print_supplement),
        help='Prints supplement data',
        aliases=['p'],
    )
    p.add_argument('name', type=str, nargs='+')
    p.add_argument('--fields', '-f', type=str, nargs='+', dest='fields', default=None)
    p = sub.add_parser(
        str(Action.print_list),
        help='Prints supplement list data',
        aliases=['pl'],
    )
    p.add_argument('name', type=str, nargs='+')
    p.add_argument('--fields', '-f', type=str, nargs='+', dest='fields', default=None)
    p = sub.add_parser(
        str(Action.print_lists),
        help='Prints all lists',
        aliases=['pal'],
    )
    p.add_argument('--name', '-n', type=str, dest='name', default=None)
    p.add_argument('--fields', '-f', type=str, nargs='+', dest='fields', default=None)
    p = sub.add_parser(
        str(Action.print_supplements),
        help='Prints all supplements',
        aliases=['pa'],
    )
    p.add_argument('--name', '-n', type=str, dest='name', default=None)
    p.add_argument('--fields', '-f', type=str, nargs='+', dest='fields', default=None)
    p = sub.add_parser(
        str(Action.add_supplement),
        help='Add supplement',
        aliases=['a'],
    )
    Supplement.populate_argparse(p, True)
    p.add_argument('--dry_run', '-r', dest='dry_run', default=False, action='store_const', const=True)
    p = sub.add_parser(
        str(Action.add_list),
        help='Add list',
        aliases=['al'],
    )
    SupplementList.populate_argparse(p, True)
    p.add_argument('--dry_run', '-r', dest='dry_run', default=False, action='store_const', const=True)
    self.db = Database(os.environ['HOME'] + '/Dropbox/notes/supp')
  def run(self):
    self.args = self.ap.parse_args()
    if self.args.action not in self.actions:
      sys.exit(f'{self.args.action} not implemented')
    self.actions[self.args.action]()

  def add_item(self, cls):
    i = cls.load(self.args.__dict__)
    if self.args.dry_run:
      print(json.dumps(i.serialize()))
      return
    self.db.add_item(cls, i)
    self.db.dump()

  def print_item(self, cls):
    for n in self.args.name:
      s = self.db.get_item(cls, n)
      if s is None:
        sys.exit(f'{cls} {n} does not exist')
      ser = s.serialize(self.args.fields)
      print(json.dumps(ser, indent=2))

  def print_items(self, cls):
    r = None
    if self.args.name:
      r = re.compile(self.args.name)
    for s in self.db.get_items(cls):
      if r and not r.search(s.name):
        continue
      print(json.dumps(s.serialize(self.args.fields)))

if __name__ == '__main__':
  Main().run()
