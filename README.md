# Corvax: a chess engine

Corvax is a chess engine I developed in the autumn of 2015. I didn't really
know what I was doing, so it reached the stage where a complete rewrite would
be easier than continuing to develop it. Instead I got bored and did something
else.

Some features:
    * Plays legal chess, apart from some complications to do with the
      fifty-move rule.
    * A vaguely xboard-compatible interface.
    * Lots of sexy search speedups, including killer moves, aspiration search,
      late move reduction, futility pruning, principal variation search, null
      move heuristic, etc.
    * An evaluation function which is not completely terrible.
    * Scores 295/300 on the Win At Chess test set.
