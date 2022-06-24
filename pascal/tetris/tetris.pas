program tetris; { tetris.pas }
uses unix, crt, score, controls, shape, field;
const
    FrameDuration = 10; { in ms }
    RegularMovementFrames = 50;
    ActionFrames = 5;
    InitDelayFrames = 70;
    NewFigureDelayFrames = 25;
    LineScoreValue = 10;

procedure NextFrame;
begin
    delay(FrameDuration)
end;

procedure SkipFrames(n: integer);
begin
    delay(FrameDuration * n)
end;

label
    StartGame;
var
    f: TetrisField;
    s: ShapePtr;
    buf: ShapeBuffer;
    action, QueuedAction: ControlsAction;
    FramesSinceMoved, FramesSinceAction, ReleaseWindowFrames: integer;
    SaveTextAttr, sc, NumLines: integer;
    ok: boolean;
begin
    fpsystem('tput civis');
    SaveTextAttr := TextAttr;
    randomize;
    clrscr;
    FieldCheckTerminalWindow;
    FieldDrawBounds;
    FieldInit(f);
    ShapeReadFromFile('figures/figure1', buf[1]);
    ShapeReadFromFile('figures/figure2', buf[2]);
    ShapeReadFromFile('figures/figure3', buf[3]);
    ShapeReadFromFile('figures/figure4', buf[4]);
StartGame:
    FieldClear(f);
    ScoreReset(sc); 
    s := nil;
    QueuedAction := Idle;
    ScoreDraw(sc);
    FramesSinceMoved := 0;
    FramesSinceAction := 0;
    ReleaseWindowFrames := -1;
    SkipFrames(InitDelayFrames);
    while true do
    begin
        if s = nil then
            ShapeSpawn(buf, s, f, ok);
        ControlsGet(action);
        if action = PressExit then
            break;
        if action <> Idle then
            QueuedAction := action;
        FramesSinceMoved := FramesSinceMoved + 1;
        FramesSinceAction := FramesSinceAction + 1;
        if ReleaseWindowFrames >= 0 then
            ReleaseWindowFrames := ReleaseWindowFrames + 1;
        if FramesSinceAction >= ActionFrames then
        begin
            case QueuedAction of
                PressLeft:
                    ShapeMove(MoveLeft, s^, f, ok);
                PressRight:
                    ShapeMove(MoveRight, s^, f, ok);
                PressDown: begin
                    ShapeMove(MoveDown, s^, f, ok);
                    FramesSinceMoved := 0
                end;
                PressRotate:
                    ShapeRotate(s^, f, ok)
            end;
            FramesSinceAction := 0;
            QueuedAction := Idle
        end;
        if FramesSinceMoved >= RegularMovementFrames then
        begin
            ShapeMove(MoveDown, s^, f, ok);
            FramesSinceMoved := 0
        end;
        NextFrame;
        if ShapeIsOnSurface(s^, f) then
        begin
            if ReleaseWindowFrames = -1 then
            begin
                ReleaseWindowFrames := 0;
                continue
            end
            else if ReleaseWindowFrames >= NewFigureDelayFrames then
            begin
                ShapeRelease(s^, f);
                FieldDeleteFullRows(NumLines, f);
                ScoreIncrement(LineScoreValue * NumLines, sc);
                ScoreDraw(sc);
                SkipFrames(NewFigureDelayFrames);
                FramesSinceMoved := 0;
                FramesSinceAction := 0;
                ReleaseWindowFrames := -1;
                ShapeSpawn(buf, s, f, ok);
                if not ok then
                    goto StartGame 
            end
        end
        else if ReleaseWindowFrames >= 0 then
            ReleaseWindowFrames := -1
    end;
    SkipFrames(InitDelayFrames);
    FieldClear(f);
    FieldDeinit(f);
    fpsystem('tput cnorm');
    TextAttr := SaveTextAttr
end.
