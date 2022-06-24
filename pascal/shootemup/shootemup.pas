program ShootEmUp; { shootemup.pas }
uses crt, Unix;
const
	DelayDuration = 5; { 10 ms, delay for the whole cycle }
	ShipLine1 = '**';
	ShipLine2 = ' ** ';
	ShipLine3 = '* ** *';
	ShipLine4 = '**    **';
	EmptyShipLine1 = '  ';
	EmptyShipLine2 = '    ';
	EmptyShipLine3 = '      ';
	EmptyShipLine4 = '        ';
	AsteroidLine14 = '**';
	AsteroidLine23 = '******';
	EmptyAsteroidLine14 = '  ';
	EmptyAsteroidLine23 = '      ';
	MaxAsteroids = 12;
	MinAsteroidDistance = 8;
	ShipHeight = 4;
	ShipNoseWidth = 2;
	BulletSymbol = '^';
	BulletMoveFreq = 2;
	AsteroidMoveFreq = 6;
	AsteroidSpawnFreq = 32;
	EndGameMessage = 'Game Over!';
	ScoreMessageBase = 'Your score: ';
    ScreenEdgeScoreIncrement = 1;
	ShootScoreIncrement = 5;

type
	ship = record
		CurX, CurY: integer;
		IsHidden: boolean;
	end;
	bullet = record
		CurX, CurY, dx, dy: integer;
		IsHidden: boolean;
	end;
	BulletArray = array [1..100] of bullet;
	asteroid = record
		CurX, CurY, dx, dy: integer;
		IsHidden: boolean;
	end;
	AsteroidArray = array [1..MaxAsteroids] of asteroid;

var { global variables }
	score: integer;

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

procedure ClampShipCoordinates(var s: ship);
begin
	if s.CurY < 1 then
		s.CurY := 1;
	if s.CurY > ScreenHeight - 1 + ShipHeight then
		s.CurY := ScreenHeight - 1 + ShipHeight;
end;

procedure HideShip(var s: ship);
begin
	GotoXY(s.CurX, s.CurY);
	write(EmptyShipLine1);
	GotoXY(s.CurX - 1, s.CurY + 1);
	write(EmptyShipLine2);
	GotoXY(s.CurX - 2, s.CurY + 2);
	write(EmptyShipLine3);
	GotoXY(s.CurX - 3, s.CurY + 3);
	write(EmptyShipLine4);
	GotoXY(1, 1)
end;


procedure ShowShip(var s: ship);
begin
	GotoXY(s.CurX, s.CurY);
	write(ShipLine1);
	GotoXY(s.CurX - 1, s.CurY + 1);
	write(ShipLine2);
	GotoXY(s.CurX - 2, s.CurY + 2);
	write(ShipLine3);
	GotoXY(s.CurX - 3, s.CurY + 3);
	write(ShipLine4);
	GotoXY(1, 1)
end;

procedure DestroyShip(var s: ship);
begin
	s.IsHidden := true;
	HideShip(s)
end;

procedure MoveShip(var s: ship; dx, dy: integer);
begin
	HideShip(s);
	s.CurX := s.CurX + dx;
	s.CurY := s.CurY + dy;
	ClampShipCoordinates(s);
	ShowShip(s)
end;

procedure HideAsteroid(var a: asteroid);
begin
	GotoXY(a.CurX, a.CurY);
	write(EmptyAsteroidLine14);
	GotoXY(a.CurX - 2, a.CurY + 1);
	write(EmptyAsteroidLine23);
	GotoXY(a.CurX - 2, a.CurY + 2);
	write(EmptyAsteroidLine23);
	GotoXY(a.CurX, a.CurY + 3);
	write(EmptyAsteroidLine14);
	GotoXY(1, 1)
end;

procedure ShowAsteroid(var a: asteroid);
begin
	GotoXY(a.CurX, a.CurY);
	write(AsteroidLine14);
	GotoXY(a.CurX - 2, a.CurY + 1);
	write(AsteroidLine23);
	GotoXY(a.CurX - 2, a.CurY + 2);
	write(AsteroidLine23);
	GotoXY(a.CurX, a.CurY + 3);
	write(AsteroidLine14);
	GotoXY(1, 1)
end;

function AsteroidIsStaticOrHidden(a: asteroid): boolean;
begin
	AsteroidIsStaticOrHidden := ((a.dx = 0) and (a.dy = 0)) or a.IsHidden
end;

procedure DestroyAsteroid(var a: asteroid);
begin
	score := score + ScreenEdgeScoreIncrement;
	HideAsteroid(a);
	a.IsHidden := true;
	a.dy := 0
end;

procedure DestroyAsteroidOnScreenEdge(var a: asteroid);
begin
	if a.CurY >= ScreenHeight - 3 then
		DestroyAsteroid(a)
end;

procedure MoveAsteroid(var a: asteroid);
begin
	if AsteroidIsStaticOrHidden(a) then
		exit;
	HideAsteroid(a);
	a.CurX := a.CurX + a.dx;
	a.CurY := a.CurY + a.dy;
	ShowAsteroid(a);
	DestroyAsteroidOnScreenEdge(a)
end;

procedure SpawnAsteroid(var a: asteroid; x: integer);
begin
	if (x < 2) or (x > ScreenWidth - 2) then
		exit;
	HideAsteroid(a);
	a.CurX := x;
	a.CurY := -2;
	a.dy := 1;
	a.IsHidden := false
end;

function AsteroidsTooClose(var a: AsteroidArray; x: integer): boolean;
var
	i: integer;
begin
	for i := 1 to MaxAsteroids do
	begin
		if (not a[i].IsHidden) and (abs(a[i].CurX - x) < MinAsteroidDistance) then
		begin
			AsteroidsTooClose := true;
			exit
		end
	end;
	AsteroidsTooClose := false
end;

function CountLiveAsteroids(var a: AsteroidArray): integer;
var
	i: integer;
begin
	CountLiveAsteroids := 0;
	for i := 1 to MaxAsteroids do
	begin
		if not a[i].IsHidden then
			CountLiveAsteroids := CountLiveAsteroids + 1
	end
end;

procedure HandleAsteroidSpawning(var a: AsteroidArray; var AsteroidNum: integer; FrameCount: integer);
var
	x: integer;
begin
	if FrameCount mod AsteroidSpawnFreq <> 0 then
		exit;
	if CountLiveAsteroids(a) >= MaxAsteroids then
		exit;
	x := random(ScreenWidth - 4) + 2;
	while AsteroidsTooClose(a, x) do
		x := random(ScreenWidth - 4) + 2;
	AsteroidsTooClose(a, x);
	SpawnAsteroid(a[AsteroidNum], x);
	AsteroidNum := AsteroidNum + 1;
	if AsteroidNum > MaxAsteroids then
		AsteroidNum := 1
end;

procedure HideBullet(var b: bullet);
begin
	GotoXY(b.CurX, b.CurY);
	write(' ');
	GotoXY(1, 1)
end;

procedure ShowBullet(var b: bullet);
begin
	GotoXY(b.CurX, b.CurY);
	write(BulletSymbol);
	GotoXY(1, 1)
end;

function BulletIsStaticOrHidden(b: bullet): boolean;
begin
	BulletIsStaticOrHidden := ((b.dx = 0) and (b.dy = 0)) or b.IsHidden
end;	

procedure DestroyBullet(var b: bullet);
begin
	HideBullet(b);
	b.IsHidden := true;
	b.dy := 0
end;

procedure DestroyBulletOnScreenEdge(var b: bullet);
begin
	if b.CurY <= 1 then
		DestroyBullet(b)
end;

procedure MoveBullet(var b: bullet);
begin
	if BulletIsStaticOrHidden(b) then
		exit;
	HideBullet(b);
	b.CurX := b.CurX + b.dx;
	b.CurY := b.CurY + b.dy;
	ShowBullet(b);
	DestroyBulletOnScreenEdge(b)
end;

procedure Spawn2Bullets(var b1, b2: bullet; var s: ship);
begin
	HideBullet(b1);
	b1.CurX := s.CurX - 1;
	b1.CurY := s.CurY;
	b1.IsHidden := false;
	b1.dy := -1;
	ShowBullet(b1);
	HideBullet(b2);
	b2.CurX := s.CurX + 2;
    b2.CurY := s.CurY;
	b2.IsHidden := false;
	b2.dy := -1;
	ShowBullet(b2)
end;

procedure Shoot(var b1, b2: BulletArray; var BulletNum: integer; var s: ship);
begin
	Spawn2Bullets(b1[BulletNum], b2[BulletNum], s);
	BulletNum := BulletNum + 1;
	if BulletNum > ScreenHeight then
		BulletNum := 1
end;

function PointInAsteroid(var a: Asteroid; x, y: integer): boolean;
begin
	PointInAsteroid := ((x >= a.CurX) and (x <= a.CurX + 1) and ((y = a.CurY + 3) or (y = a.CurY))) or
		((x >= a.CurX - 2) and (x <= a.CurX + 3) and (y >= a.CurY + 1) and (y <= a.CurY + 2))
end;

procedure HandleBulletCollision(var a: AsteroidArray; var b: bullet);
var
	i: integer;
begin
	for i := 1 to MaxAsteroids do
	begin
		if a[i].IsHidden or b.IsHidden then
			continue;
		if not PointInAsteroid(a[i], b.CurX, b.CurY) then
			continue;
		score := score + ShootScoreIncrement;
		DestroyBullet(b);
		DestroyAsteroid(a[i]);
		break
	end
end;

function PointInShip(var s: ship; x, y: integer): boolean;
begin
	PointInShip := ((x >= s.CurX) and (x <= s.CurX + 1) and (y >= s.CurY) and (y <= s.CurY + 1)) or
		((x >= s.CurX - 2) and (x <= s.CurX + 3) and (y = s.CurY + 2)) or
		((x >= s.CurX - 3) and (x <= s.CurX + 4) and (y = s.CurY + 3))
end;

procedure HandleShipToAsteroidCollision(var s: ship; var a: asteroid);
label
	Destroy;
var
	x, y: integer;
begin
	if s.IsHidden or a.IsHidden then
		exit;
	x := a.CurX;
	y := a.CurY + 3;
	if PointInShip(s, x, y) then
		goto Destroy;
	x := a.CurX + 1;
	if PointInShip(s, x, y) then
		goto Destroy;
	x := a.CurX - 2;
	y := a.CurY + 2;
	if PointInShip(s, x, y) then
		goto Destroy;
	x := a.CurX + 3;
	if PointInShip(s, x, y) then
		goto Destroy;
	exit;
	Destroy:
		DestroyAsteroid(a);
	DestroyShip(s)
end;

procedure InitializeBullets(var b1, b2: BulletArray; var BulletNum: integer);
var
	i: integer;
begin
	BulletNum := 1;
	for i := 1 to ScreenHeight do
	begin
		b1[i].IsHidden := true;
		b1[i].dx := 0;
		b2[i].IsHidden := true;
		b2[i].dx := 0
	end
end;

procedure InitializeAsteroids(var a: AsteroidArray; var AsteroidNum: integer);
var
	i: integer;
begin
	AsteroidNum := 1;
	for i := 1 to MaxAsteroids do
	begin
		a[i].IsHidden := true;
		a[i].dx := 0
	end
end;

procedure HandleBulletMovement(var b1, b2: BulletArray; FrameCount: integer);
var
	i: integer;
begin
	if FrameCount mod BulletMoveFreq <> 0 then
		exit;
	for i := 1 to ScreenHeight do
	begin
		if not b1[i].IsHidden then
			MoveBullet(b1[i]);
		if not b2[i].IsHidden then
			MoveBullet(b2[i])
	end
end;

procedure HandleAsteroidMovement(var a: AsteroidArray; FrameCount: integer);
var
	i: integer;
begin
	if FrameCount mod AsteroidMoveFreq <> 0 then
		exit;
	for i := 1 to MaxAsteroids do
	begin
		if not a[i].IsHidden then
			MoveAsteroid(a[i])
	end
end;

procedure HandleAllBulletCollisions(var a: AsteroidArray; var b1, b2: BulletArray);
var
	i: integer;
begin
	for i := 1 to ScreenHeight do
	begin
		HandleBulletCollision(a, b1[i]);
		HandleBulletCollision(a, b2[i])
	end
end;

procedure HandleShipCollisions(var s: ship; var a: AsteroidArray);
var
	i: integer;
begin
	for i := 1 to MaxAsteroids do
	begin
		HandleShipToAsteroidCollision(s, a[i]);
		if s.IsHidden then
			exit
	end
end;

procedure EndGame;
var
	x, y, c: integer;
	hs: string;
begin
	clrscr;
	x := (ScreenWidth - length(EndGameMessage)) div 2;
	y := ScreenHeight div 2;
	GotoXY(x, y);
	write(EndGameMessage);
	str(score, hs);
	hs := ScoreMessageBase + hs;
	x := (ScreenWidth - length(hs)) div 2;
	y := y + 1;
	GotoXY(x, y);
	write(hs);
	GotoXY(1, 1);
	while not KeyPressed do { stupid solution }
		delay(DelayDuration);
	while true do
	begin
		if not KeyPressed then
			continue;
		GetKey(c);
		if c = 27 then
			exit
	end
end;	

label
	Start;
var
	s: ship;
	b1, b2: BulletArray;
	a: AsteroidArray;
	FrameCount, c, BulletNum, AsteroidNum: integer;
begin
	fpSystem('tput civis');
Start:
	clrscr;
	FrameCount := 0;
	score := 0;
	s.IsHidden := false;
	s.CurX := (ScreenWidth - ShipNoseWidth) div 2;
	s.CurY := ScreenHeight div 2;
	InitializeBullets(b1, b2, BulletNum);
	InitializeAsteroids(a, AsteroidNum);
	ShowShip(s);
	while true do
	begin
		delay(DelayDuration);
		HandleAsteroidSpawning(a, AsteroidNum, FrameCount);
		HandleBulletMovement(b1, b2, FrameCount);
		HandleAsteroidMovement(a, FrameCount);
		HandleAllBulletCollisions(a, b1, b2);
		HandleShipCollisions(s, a);
		if s.IsHidden then
		begin
			EndGame;
			goto Start
		end;
		FrameCount := FrameCount + 1;
		if not KeyPressed then
			continue;
		GetKey(c);
		case c of
			97: { left arrow }
				MoveShip(s, -1, 0);
			100: { right arrow }
				MoveShip(s, 1, 0);
			119: { up arrow }
				MoveShip(s, 0, -1);
			115: { down arrow }
				MoveShip(s, 0, 1);
			32: { space }
				Shoot(b1, b2, BulletNum, s);
			27: { escape }
				break
		end
	end;
	clrscr;
	fpSystem('tput cnorm')
end.	
