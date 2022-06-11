unit field; { field.pp }
interface

const
    FieldWidth = 10;
    FieldHeight = 15;

type
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
procedure FieldDrawBounds;
procedure FieldInit(var field: TetrisField);
procedure FieldDrawSquare(i, j: integer; var field: TetrisField);
procedure FieldHideSquare(i, j: integer; var field: TetrisField);
function FieldPointIsInBounds(i, j: integer): boolean;
procedure FieldClear(var field: TetrisField);

implementation
uses crt;
const
    BoundsSideSymbol = '|';
    BoundsBottomSymbol = '-';
    SquareFillSymbol = '#';
    SquareWidth = 3;
    SquareHeight = 2;
    BoundsWidth = 1;
    UIPlackSize = 1;

function RealFieldWidth: integer;
begin
    RealFieldWidth := FieldWidth * SquareWidth + 2 * BoundsWidth
end;

function RealFieldHeight: integer;
begin
    RealFieldHeight := 
        FieldHeight * SquareHeight + BoundsWidth + UIPlackSize;
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

procedure FieldDrawBounds;
var
    x, y: integer;
begin
    { Should I add implementation for bounds thicker than 1? }
    x := RealFieldWidth;
    for y := 1 + UIPlackSize to RealFieldHeight do
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

function FieldPointIsInBounds(i, j: integer): boolean;
begin
    FieldPointIsInBounds := (i >=  1) and (j >= 1) and
        (i <= FieldWidth) and (j <= FieldHeight)
end;

procedure FillSquare(symbol: char; i, j: integer);
var
    InitX, x, InitY, y: integer;
begin
    InitX := BoundsWidth + (i - 1) * SquareWidth + 1;
    InitY := RealFieldHeight - BoundsWidth - (j - 1) * SquareHeight;
    for y := InitY downto InitY - SquareHeight + 1 do
    begin
        GotoXY(InitX, y);
        for x := InitX to InitX + SquareWidth - 1 do
            write(symbol)
    end;
    GotoXY(1, 1)
end;

procedure FieldDrawSquare(i, j: integer; var field: TetrisField);
begin
    if not field.IsInitialized then
    begin
        {$IFDEF DEBUG}
        writeln('DEBUG: trying to draw to unitialized field');
        {$ENDIF DEBUG}
        exit
    end;
    if not FieldPointIsInBounds(i, j) then
    begin
        {$IFDEF DEBUG}
        writeln(
            'DEBUG: trying to draw square out of bounds, x: ', i, ', y: ', j);
        {$ENDIF DEBUG}
        exit
    end;
    TextColor(field.squares[i][j].color);
    FillSquare(SquareFillSymbol, i, j);
    field.squares[i][j].IsFilled := true
end;

procedure FieldHideSquare(i, j: integer; var field: TetrisField);
begin
    if not FieldPointIsInBounds(i, j) then
    begin
        {$IFDEF DEBUG}
        writeln(
            'DEBUG: trying to hide square out of bounds, x: ', i, ', y: ', j);
        {$ENDIF DEBUG}
        exit
    end;
    FillSquare(' ', i, j);
    field.squares[i][j].IsFilled := false;
    field.squares[i][j].IsInFigure := false
end;

procedure FieldClear(var field: TetrisField);
var
    i, j: integer;
begin
    field.IsInitialized := false;
    for i := 1 to FieldWidth do
        for j := 1 to FieldHeight do
            FieldHideSquare(i, j, field)
end;

end.
