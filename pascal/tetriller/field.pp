unit field; { field.pp }
interface

const
    FieldWidth = 10;
    FieldHeight = 15;
    FieldSquareHeight = 2;

type
    FieldPosition = array [1..2] of integer;
    FieldLines = array [1..FieldSquareHeight] of string;
    FieldSquare = record
        color: word;
        IsFilled, IsInFigure: boolean;
    end;
    TetrisField = record 
        squares: array [1..FieldWidth] of
            array [1..FieldHeight] of FieldSquare;
        IsInitialized: boolean;
    end;

procedure FieldCheckTerminalWindow;
procedure FieldDrawBounds(WinHeight: integer);
procedure FieldSetPosition(i, j: integer; var p: FieldPosition);
procedure FieldModifyPosition(di, dj: integer; var p: FieldPosition);
procedure FieldInit(var field: TetrisField);
procedure FieldDrawSquare(p: FieldPosition; color: word;
    IsFigure: boolean; var field: TetrisField);
procedure FieldHideSquare(p: FieldPosition; var field: TetrisField);
procedure FieldGetSquare(p: FieldPosition; 
    var field: TetrisField; var square: FieldSquare);
procedure FieldReleaseSquare(p: FieldPosition; var field: TetrisField);
function FieldPointIsInBounds(p: FieldPosition): boolean;
procedure FieldClear(var field: TetrisField);
procedure FieldDeinit(var field: TetrisField);
procedure FieldDrawSquareCustom(
    p: FieldPosition; 
    var lines: FieldLines;     
    var field: TetrisField
);
procedure FieldHideSquareCustom(p: FieldPosition; var field: TetrisField);

implementation
uses crt;
const
    BoundsSideSymbol = '|';
    BoundsBottomSymbol = '-';
    SquareFillSymbol = '#';
    FinishSymbol = '*';
    SquareWidth = 3;
    BoundsWidth = 1;
    UIPlackSize = 1;

function RealFieldWidth: integer;
begin
    RealFieldWidth := FieldWidth * SquareWidth + 2 * BoundsWidth
end;

function RealFieldHeight: integer;
begin
    RealFieldHeight := 
        FieldHeight * FieldSquareHeight + BoundsWidth + UIPlackSize;
end;

procedure FieldCheckTerminalWindow;
begin
    if (ScreenWidth < RealFieldWidth) or
        (ScreenHeight < RealFieldHeight) then
    begin
        writeln(ErrOutput,
            'Terminal window is too small, please resize before playing');
        halt(1)
    end
end;

procedure FieldDrawBounds(WinHeight: integer);
var
    x, y: integer;
begin
    { Should I add implementation for bounds thicker than 1? }
    x := RealFieldWidth;
    for y := 1 + UIPlackSize to 
        FieldSquareHeight * (FieldHeight - WinHeight) + UIPlackSize do
    begin
        GotoXY(1, y);
        write(BoundsSideSymbol);
        GotoXY(x, y);
        write(BoundsSideSymbol)
    end;
    for y := FieldSquareHeight * (FieldHeight - WinHeight) + UIPlackSize + 1 to
        FieldSquareHeight * (FieldHeight - WinHeight + 1) + UIPlackSize do
    begin
        GotoXY(1, y);
        write(FinishSymbol);
        GotoXY(x, y);
        write(FinishSymbol)
    end;
    for y := FieldSquareHeight * (FieldHeight - WinHeight + 1) +
        UIPlackSize + 1 to RealFieldHeight do
    begin
        GotoXY(1, y);
        write(BoundsSideSymbol);
        GotoXY(x, y);
        write(BoundsSideSymbol)
    end;
    GotoXY(1, RealFieldHeight);
    for x := 1 to RealFieldWidth do
        write(BoundsBottomSymbol);
    GotoXY(1, 1)
end;

procedure FieldSetPosition(i, j: integer; var p: FieldPosition);
begin
    p[1] := i;
    p[2] := j
end;

procedure FieldModifyPosition(di, dj: integer; var p: FieldPosition);
begin
    FieldSetPosition(p[1] + di, p[2] + dj, p)
end;

procedure FieldInit(var field: TetrisField);
var
    i, j: integer;
begin
    field.IsInitialized := true;
    for i := 1 to FieldWidth do
        for j := 1 to FieldHeight do
        begin
            field.squares[i][j].color := white;
            field.squares[i][j].IsFilled := false;
            field.squares[i][j].IsInFigure := false
        end
end;

function FieldPointIsInBounds(p: FieldPosition): boolean;
begin
    FieldPointIsInBounds := (p[1] >=  1) and (p[2] >= 1) and
        (p[1] <= FieldWidth) and (p[2] <= FieldHeight)
end;

procedure FillSquare(symbol: char; i, j: integer);
var
    InitX, x, InitY, y: integer;
begin
    InitX := BoundsWidth + (i - 1) * SquareWidth + 1;
    InitY := RealFieldHeight - BoundsWidth - (j - 1) * FieldSquareHeight;
    for y := InitY downto InitY - FieldSquareHeight + 1 do
    begin
        GotoXY(InitX, y);
        for x := InitX to InitX + SquareWidth - 1 do
            write(symbol)
    end;
    GotoXY(1, 1)
end;

procedure FieldDrawSquareCustom(
    p: FieldPosition; 
    var lines: FieldLines;     
    var field: TetrisField
);
var
    InitX, i, InitY, y: integer;
begin
    if not field.IsInitialized then
        exit;
    if not FieldPointIsInBounds(p) then
        exit;
    if field.squares[p[1]][p[2]].IsFilled then
        exit;
    InitX := BoundsWidth + (p[1] - 1) * SquareWidth + 1;
    InitY := RealFieldHeight - BoundsWidth - (p[2] - 1) * FieldSquareHeight;
    i := 1;
    for y := InitY downto InitY - FieldSquareHeight + 1 do
    begin
        GotoXY(InitX, y);
        write(copy(lines[i], 1, SquareWidth));
        i := i + 1
    end;
    GotoXY(1, 1)
end;

procedure FieldDrawSquare(p: FieldPosition; color: word;
    IsFigure: boolean; var field: TetrisField);
begin
    if not field.IsInitialized then
        exit;
    if not FieldPointIsInBounds(p) then
        exit;
    TextColor(color);
    FillSquare(SquareFillSymbol, p[1], p[2]);
    field.squares[p[1]][p[2]].color := color;
    field.squares[p[1]][p[2]].IsFilled := true;
    field.squares[p[1]][p[2]].IsInFigure := IsFigure
end;

procedure FieldHideSquareCustom(p: FieldPosition; var field: TetrisField);
begin
    if not FieldPointIsInBounds(p) then
        exit;
    FillSquare(' ', p[1], p[2])
end;

procedure FieldHideSquare(p: FieldPosition; var field: TetrisField);
begin
    FieldHideSquareCustom(p, field);
    field.squares[p[1]][p[2]].color := white;
    field.squares[p[1]][p[2]].IsFilled := false;
    field.squares[p[1]][p[2]].IsInFigure := false
end;

procedure FieldGetSquare(p: FieldPosition; 
    var field: TetrisField; var square: FieldSquare);
begin
    if not FieldPointIsInBounds(p) then
        exit;
    square := field.squares[p[1]][p[2]]
end;

procedure FieldReleaseSquare(p: FieldPosition; var field: TetrisField);
begin
    if not FieldPointIsInBounds(p) then
        exit;
    field.squares[p[1]][p[2]].IsInFigure := false
end;

procedure FieldClear(var field: TetrisField);
var
    i, j: integer;
    p: FieldPosition;
begin
    for i := 1 to FieldWidth do
        for j := 1 to FieldHeight do
        begin
            FieldSetPosition(i, j, p);
            FieldHideSquare(p, field)
        end
end;

procedure FieldDeinit(var field: TetrisField);
begin
    field.IsInitialized := false;
    clrscr
end;

end.
