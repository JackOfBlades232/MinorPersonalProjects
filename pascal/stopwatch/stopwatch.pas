program PseudoStopwatch; { stopwatch.pas }
uses crt;
const
    second = 1000;
    tick = 1; { 1 sec }
    template = '00:00';
    MaxTime = 3599;
    FirstRank = 600;
    SecondRank = 60;
    ThirdRank = 10;
type
    StopwatchMode = (inactive, playing, stopped);
    stopwatch = record
        mode: StopwatchMode;
        time: integer;
    end;

procedure GetKey(var code: integer);
var
    c: char;
begin
    c := ReadKey;
    if c = #0 then
    begin
        c := ReadKey;
        code := -ord(c)
    end
    else
    begin
        code := ord(c)
    end
end;

function Num2Chr(n: integer): char;
begin
    Num2Chr := chr(n + ord('0'))
end;
    
procedure WriteTime(time: integer);
begin
    if time > MaxTime then
        time := MaxTime;
    write(Num2Chr(time div FirstRank));
    time := time mod FirstRank;
    write(Num2Chr(time div SecondRank));
    time := time mod SecondRank;
    write(':');
    write(Num2Chr(time div ThirdRank));
    write(Num2Chr(time mod ThirdRank));
end;

procedure DrawStopwatch(var sw: stopwatch; x, y: integer);
begin
    GotoXY(x, y);
    WriteTime(sw.time);
    GotoXY(1, 1)
end;

procedure RestartStopwatch(var sw: stopwatch);
begin
    sw.mode := inactive;
    sw.time := 0
end;

procedure IncrementStopwatchTime(var sw: stopwatch);
begin
    delay(tick * second);
    sw.time := sw.time + tick
end;

procedure HandleEscape(var sw: stopwatch; x, y: integer);
begin
    if sw.mode = inactive then
    begin
        clrscr;
        halt(0)
    end;
    RestartStopwatch(sw);
    DrawStopwatch(sw, x, y)
end;

procedure HandleSpace(var sw: stopwatch; x, y: integer);
begin
    if sw.mode = playing then
        sw.mode := stopped
    else
        sw.mode := playing;
    DrawStopwatch(sw, x, y)
end;

procedure HandlePlaying(var sw: stopwatch; x, y: integer);
begin
    if sw.mode = playing then
    begin
        IncrementStopwatchTime(sw);
        DrawStopwatch(sw, x, y)
    end
end;

label
    HandleLogic;
var
    sw: stopwatch;
    x, y, c: integer;
begin
    clrscr;
    x := (ScreenWidth - length(template)) div 2;
    y := ScreenHeight div 2;
    RestartStopwatch(sw);
    DrawStopwatch(sw, x, y);
    while true do
    begin
        if not KeyPressed then
            goto HandleLogic;
        GetKey(c);
        case c of
            27: { escape }
                HandleEscape(sw, x, y);
            32: { space }
                HandleSpace(sw, x, y)
        end;
        HandleLogic:
            HandlePlaying(sw, x, y)
    end
end.
