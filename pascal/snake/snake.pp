unit snake; { snake.pp }
interface
const
    SnakeFoodBufferSize = 4;
type
    SnakeMoveDirection = (up, right, down, left);
    SnakeMoveResult = (eat, success, fail);
    SnakeNodePtr = ^SnakeNode;
    SnakeNode = record
        x, y: integer;
        next: SnakeNodePtr;
    end;
    SnakeHead = record
        dx, dy: integer;
        node: SnakeNodePtr;
    end;
    SnakeFoodPiece = record
        x, y: integer;
        IsSpawned: boolean;
    end;
    SnakeFoodBuffer = array [1..SnakeFoodBufferSize] of SnakeFoodPiece;

procedure SnakeSpawn(var s: SnakeHead);
procedure SnakeFoodBufferInit(var buf: SnakeFoodBuffer);
procedure SnakeSpawnFood(var s: SnakeHead; var buf: SnakeFoodBuffer);
procedure SnakeCollectFood(x, y: integer; 
    var s: SnakeHead; var buf: SnakeFoodBuffer);
procedure SnakeSetDirection(dir: SnakeMoveDirection; var s: SnakeHead);
procedure SnakeMove(var s: SnakeHead;
    var buf: SnakeFoodBuffer; var res: SnakeMoveResult);
procedure SnakeDespawn(var s: SnakeHead);

implementation
uses crt;
const
    HeadSymbol = '@';
    BodySymbol = 'o';
    FoodSymbol = '#';

procedure WrapCoordinates(var x, y: integer);
begin
    while x < 1 do
        x := x + ScreenWidth;
    while y < 1 do
        y := y + ScreenHeight;
    while x > ScreenWidth do
        x := x - ScreenWidth;
    while y > ScreenHeight do
        y := y +  ScreenHeight
end;

procedure DrawSymbol(x, y: integer; c: char);
begin
    WrapCoordinates(x, y);
    GotoXY(x, ScreenHeight + 1 - y);
    write(c);
    GotoXY(1, 1)
end;

function PointIsFood(x, y: integer; var buf: SnakeFoodBuffer): boolean;
var
    i: integer;
begin
    WrapCoordinates(x, y);
    PointIsFood := false;
    for i := 1 to SnakeFoodBufferSize do
        if buf[i].IsSpawned and (buf[i].x = x) and (buf[i].y = y) then
        begin
            PointIsFood := true;
            break
        end
end;

function PointIsInSnake(x, y: integer; var s: SnakeHead): boolean;
var
    tmp: SnakeNodePtr;
begin
    WrapCoordinates(x, y);
    PointIsInSnake := false;
    if s.node = nil then
        exit;
    tmp := s.node;
    while tmp <> nil do
    begin
        if (tmp^.x = x) and (tmp^.y = y) then
        begin
            PointIsInSnake := true;
            break
        end;
        tmp := tmp^.next
    end
end;

procedure SnakeSpawnFood(var s: SnakeHead; var buf: SnakeFoodBuffer);
var
    i: integer;
begin
    for i := 1 to SnakeFoodBufferSize do
    begin
        if buf[i].IsSpawned then
            continue;
        repeat
            buf[i].x := random(ScreenWidth) + 1;
            buf[i].y := random(ScreenHeight) + 1;
        until (not PointIsFood(buf[i].x, buf[i].y, buf)) and
            (not PointIsInSnake(buf[i].x, buf[i].y, s));
        buf[i].IsSpawned := true;
        DrawSymbol(buf[i].x, buf[i].y, FoodSymbol);
        break
    end
end;

procedure HideSnakeNode(var p: SnakeNodePtr);
begin
    if p = nil then
        exit;
    DrawSymbol(p^.x, p^.y, ' ')
end;

procedure DrawSnakeNode(var p: SnakeNodePtr; var s: SnakeHead);
begin
    if (p = nil) or (s.node = nil) then
        exit;
    if s.node = p then
        DrawSymbol(p^.x, p^.y, HeadSymbol)
    else
        DrawSymbol(p^.x, p^.y, BodySymbol)
end;

procedure SnakeCollectFood(x, y: integer; var s: SnakeHead; var buf: SnakeFoodBuffer);
var
    i, OldX, OldY: integer;
    tmp: SnakeNodePtr;
begin
    if s.node = nil then
        exit;
    i := 1;
    while i <= SnakeFoodBufferSize do
    begin
        if (buf[i].x = x) and (buf[i].y = y) then
            break;
        i := i + 1
    end;
    if i > SnakeFoodBufferSize then
        exit;
    buf[i].IsSpawned := false;
    OldX := s.node^.x;
    OldY := s.node^.y;
    s.node^.x := x;
    s.node^.y := y;
    new(tmp);
    tmp^.next := s.node^.next;
    tmp^.x := OldX;
    tmp^.y := OldY;
    s.node^.next := tmp;
    DrawSnakeNode(s.node, s);
    DrawSnakeNode(tmp, s)
end;

procedure SnakeSpawn(var s: SnakeHead);
begin
    s.dx := 0;
    s.dy := 0;
    new(s.node);
    s.node^.x := ScreenWidth div 2;
    s.node^.y := ScreenHeight div 2;
    s.node^.next := nil;
    DrawSnakeNode(s.node, s)
end;

procedure SnakeFoodBufferInit(var buf: SnakeFoodBuffer);
var 
    i: integer;
begin
    for i := 1 to SnakeFoodBufferSize do
        buf[i].IsSpawned := false
end;

procedure SetSnakeDXDY(dx, dy: integer; var s: SnakeHead);
begin
    s.dx := dx;
    s.dy := dy
end;

procedure SnakeSetDirection(dir: SnakeMoveDirection; var s: SnakeHead);
begin
    if s.node = nil then
        exit;
    case dir of
        Up: 
            SetSnakeDXDY(0, 1, s);
        Right: 
            SetSnakeDXDY(1, 0, s);
        Down: 
            SetSnakeDXDY(0, -1, s);
        Left: 
            SetSnakeDXDY(-1, 0, s)
    end
end;

procedure SnakeMove(var s: SnakeHead;
    var buf: SnakeFoodBuffer; var res: SnakeMoveResult);
var
    NewX, NewY, OldX, OldY: integer;
    tmp: SnakeNodePtr;
begin
    if s.node = nil then
        exit;
    NewX := s.node^.x + s.dx;
    NewY := s.node^.y + s.dy;
    WrapCoordinates(NewX, NewY);
    if PointIsInSnake(NewX, NewY, s) then
    begin
        res := fail;
        exit
    end;
    if PointIsFood(NewX, NewY, buf) then
    begin
        SnakeCollectFood(NewX, NewY, s, buf);
        res := eat;
        exit
    end;
    tmp := s.node;
    while tmp <> nil do
    begin
        if tmp^.next = nil then
            HideSnakeNode(tmp); 
        OldX := tmp^.x;
        OldY := tmp^.y;
        tmp^.x := NewX;
        tmp^.y := NewY;
        NewX := OldX;
        NewY := OldY;
        if (tmp = s.node) or (tmp = s.node^.next) then
            DrawSnakeNode(tmp, s);
        tmp := tmp^.next
    end;
    res := success
end;

procedure SnakeDespawn(var s: SnakeHead);
var
    tmp: SnakeNodePtr;
begin
    while s.node <> nil do
    begin
        tmp := s.node^.next;
        dispose(s.node);
        s.node := tmp
    end
end;

end.
