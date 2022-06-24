unit ScoreUI; { score_ui.pp }
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
    GotoXY(1, 1);
    for i := 1 to ScreenHeight do
        write(' ');
    GotoXY(1, 1);
    score := 0
end;

procedure ScoreIncrement(step: integer; var score: integer);
begin
    if score + step >= score then
        score := score +  step
end;

procedure ScoreDraw(score: integer);
begin
    GotoXY(1, 1);
    write(score);
    GotoXY(1, 1)
end;

end.
