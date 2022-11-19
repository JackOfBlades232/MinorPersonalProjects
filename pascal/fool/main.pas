program main; { main.pas }
{
    Main cycle for fool
}
uses crt, restock, encounter, deck, hand, card;

procedure InitGame(
    var d: CardDeck; var trump: CardSuit; 
    var enc: EncounterData; var h1, h2: PlayerHand; 
    var attacker, defender: PlayerHandPtr; var ok: boolean
);
begin
    randomize;
    GenerateDeck(d);
    TryGetTrumpSuit(d, trump, ok);
    if not ok then
        exit;

    InitHand(h1);
    InitHand(h2);
    InitRestockHands(h1, h2, d, ok);
    attacker := @h1;
    defender := @h2;
end;

procedure DrawDeckInfo(var d: CardDeck; trump: CardSuit);
begin
    writeln('Trump suit: ', trump);
    writeln('Cards in deck: ', RemainingDeckSize(d))
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
            writeln(idx, '. ', h[i]^.value, ' ', h[i]^.suit);
            idx := idx + 1
        end
end;

procedure TryDrawCardPair(var c1, c2: CardPtr);
begin
    if c1 <> nil then
        write(c1^.value, ' ', c1^.suit);
    if c2 <> nil then
        write(c2^.value, ' ', c2^.suit);
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
    DrawEncounterTable(enc)
end;

procedure DeinitGame(var d: CardDeck; var enc: EncounterData;
    var h1, h2: PlayerHand);
begin
    FinishEncounter(enc);
    DisposeOfHand(h1);
    DisposeOfHand(h2);
    DisposeOfDeck(d);
    clrscr
end;
        
var
    d: CardDeck;
    trump: CardSuit;
    enc: EncounterData;
    h1, h2: PlayerHand;
    attacker, defender: PlayerHandPtr;

    ok: boolean;
begin
    InitGame(d, trump, enc, h1, h2, attacker, defender, ok);
    if not ok then
    begin
        writeln(ErrOutput, 'Failed to initialize game!');
        halt(1)
    end;

    DrawTurnInfo(d, trump, h1, enc);
    delay(30000);

    DeinitGame(d, enc, h1, h2)
end.
