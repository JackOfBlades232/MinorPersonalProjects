unit score; { score.pp }
interface

procedure ScoreReset(var score: integer);
procedure ScoreIncrement(step: integer; var score: integer);
procedure ScoreDraw(score: integer);

implementation
uses crt;

procedure ScoreReset(var score: integer);
var
    i: integer;
begin
    score := 0;
    GotoXY(1, 1);
    for i := 1 to ScreenWidth do
        write(' ');
    GotoXY(1, 1)
end;

procedure ScoreIncrement(step: integer; var score: integer);
begin
    if score + step >= score then
        score := score + step
end;

procedure ScoreDraw(score: integer);
begin
    TextColor(white);
    GotoXY(1, 1);
    write(' Score: ', score);
    GotoXY(1, 1)
end;

end.
