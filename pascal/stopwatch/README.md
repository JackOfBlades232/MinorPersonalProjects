# Stopwatch
This little program contains a stopwatch for the terminal, which you can start, pause and reset.

## State
The project is completed, except for the fact, that pause lags, because I used delays. I intend to fix this by using system time instead.

## Controls
Escape -- reset if is playing or paused, exit otherwise
Space -- play/pause

## Running
(for linux) In order to run this program, you have to compile it with the fpc compiler (`fpc stopwatch.pas`), and run the generated executable file.
