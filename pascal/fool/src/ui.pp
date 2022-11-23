unit UI; { ui.pp }
interface
uses encounter, hand, deck, card;
{
    Contains procedures for drawing the game's ui and collecting input
}

procedure DrawTurnInfo(var d: CardDeck; trump: CardSuit; 
    var h: PlayerHand; var enc: EncounterData);
procedure CollectPlayerInput(var CardIndex: integer; var ok: boolean);

implementation
uses crt;
procedure DrawDeckInfo(var d: CardDeck; trump: CardSuit);
begin
    writeln('Trump suit: ', trump);
    writeln('Cards in deck: ', RemainingDeckSize(d))
end;

procedure TryDrawCard(var c: CardPtr);
begin
    if c <> nil then
        write(c^.value, ' of ', c^.suit)
end;

procedure DrawPlayerHand(var h: PlayerHand);
var
    i, idx: integer;
begin
    writeln('Your cards:');
    idx := 1;
    for i := 1 to DeckSize do
        if h[i] <> nil then
        begin
            write(idx, '. ');
            TryDrawCard(h[i]);
            writeln;
            idx := idx + 1
        end
end;

procedure TryDrawCardPair(var c1, c2: CardPtr);
begin
    TryDrawCard(c1);
    if (c1 <> nil) or (c2 <> nil) then
        write(' : ');
    TryDrawCard(c2);
    if (c1 <> nil) or (c2 <> nil) then
        writeln
end;

procedure DrawEncounterTable(var enc: EncounterData);
var
    i: integer;
begin
    writeln('Table:');
    for i := 1 to NormalHandSize do
        TryDrawCardPair(enc.AttackerTable[i], enc.DefenderTable[i])
end;

procedure DrawTurnInfo(var d: CardDeck; trump: CardSuit; 
    var h: PlayerHand; var enc: EncounterData);
begin
    clrscr;
    GotoXY(1, 1);
    DrawDeckInfo(d, trump);
    writeln;
    DrawPlayerHand(h);
    writeln;
    DrawEncounterTable(enc);
    writeln
end;

procedure CollectPlayerInput(var CardIndex: integer; var ok: boolean);
begin
    {$I-}
    write('Input the index of the card you want to play (0=forfeit): ');
    readln(CardIndex);
    ok := (IOResult = 0) and (CardIndex >= 0)
end;

end.
