import click
from contextlib import contextmanager
import subprocess
import sys
import string
unicode_pieces = dict(zip(
    [ord(x) for x in u"KQRBNPkqrbnp"],
    u"\u2654\u2655\u2656\u2657\u2658\u2659\u265a\u265b\u265c\u265d\u265e\u265f"
    ))


def parse_fen(fen):
    parts = fen.split()
    board = {"side": parts[1]}
    boardfen = parts[0].split("/")
    assert len(boardfen) == 8, fen
    for rank in [8, 7, 6, 5, 4, 3, 2, 1]:
        line = boardfen[8 - rank]
        fil = 0
        while line:
            if line[0].isalpha():
                board["{}{}".format("abcdefgh"[fil], rank)] = line[0]
                fil += 1
            else:
                fil += int(line[0])
            line = line[1:]
    return board


def print_board(board):
    txt = u""
    for rank in [8, 7, 6, 5, 4, 3, 2, 1]:
        for fil in "abcdefgh":
            sq = "{}{}".format(fil, rank)
            txt += board.get(sq, ".")
        txt += "\n"
    print txt.translate(unicode_pieces)


def is_same_move(my_move, move, board):
    move = move.translate(None, "x+")
    piece = board[my_move[:2]].upper()
    if piece == 'P':
        piece = ''
    if move == piece + my_move[2:]:
        return True
    if move == piece + my_move[0] + my_move[2:]:
        return True
    return False


class Test(object):

    def __init__(self, fen_line):
        self.fen_line = fen_line
        self.best_moves = fen_line.split('bm ')[1].split(';')[0].split()
        self.testid = fen_line.split(";")[-2].split('"')[1]
        self.board = parse_fen(fen_line)


class Engine(object):

    def __init__(self, args, init_strings=None, pre_test=None):
        self.engine = subprocess.Popen(args, stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE)
        if init_strings:
            self.engine.stdin.write(init_strings)
            self.engine.stdin.flush()
        self.pre_test = pre_test

    def run_test(self, test, verbose=False):
        if self.pre_test:
            self.engine.stdin.write(self.pre_test)
        self.engine.stdin.write("fen " + test.fen_line + "\ngo\n")
        self.engine.stdin.flush()
        for line in iter(self.engine.stdout.readline, ''):
            line = line.strip()
            if not line:
                continue
            if not line.startswith('#'):
                break
        my_move = line.split()[1]
        self.last_move = my_move
        for move in test.best_moves:
            if is_same_move(my_move, move, test.board):
                return True
        return False

    def shutdown(self):
        self.engine.stdin.write("quit\n")
        self.engine.stdin.flush()

inits = """
"""

pre_test = """
time 30000
"""


def run_test(engine, test, verbose=False):
    result = engine.run_test(test)
    if verbose:
        print test.testid
        print test.fen_line
        print_board(test.board)
        print "White" if test.board['side'] == 'w' else "Black", "to play"
        print "Best move is", test.best_moves
        if result:
            print "Correct"
        else:
            print "Incorrect"
        print "--"
    else:
        if not result:
            print test.fen_line, "#", engine.last_move
    return result


@contextmanager
def corvax(**kwargs):
    engine = Engine("bin/corvax", **kwargs)
    yield engine
    engine.shutdown()


@click.command()
@click.option('--test-number', '-n', default=0)
@click.option('--hard/--all', '-h/-a', default=False)
@click.option('--verbose/--quiet', '-v/-q', default=False)
def main(test_number, verbose, hard):
    if hard:
        filename = "tests/hardwac.epd"
    else:
        filename = "tests/wac.epd"
    tests = [line.strip().split('#')[0] for line in open(filename).readlines()]
    tests = [Test(line) for line in tests if line]
    with corvax(pre_test=pre_test) as engine:
        if test_number == 0:
            successes = 0
            for test in tests:
                if run_test(engine, test, verbose):
                    successes += 1
        else:
            run_test(engine, tests[test_number - 1], verbose)
    print "Tests completed: {}/{} correct".format(successes, len(tests))

if __name__ == '__main__':
    main()
