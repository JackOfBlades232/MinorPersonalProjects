program tests; { tests.pas }
uses restock, encounter, hand, deck, card;
var
    i: integer;
    ok: boolean;
    inp: integer;

    card1, card2, TrumpCard: PlayingCard;

    d: CardDeck;
    p: CardPtr;
    trump: CardSuit;

    e: EncounterData;
    h1, h2: PlayerHand;
    h1p, h2p: PlayerHandPtr;
    IsAttackerTurn: boolean;
begin
    randomize;
    GenerateDeck(d);
    TryGetTrumpSuit(d, trump, ok);
{$IFDEF CARD}
    TrumpCard := RandomCard;
    card1 := RandomCard;
    card2 := RandomCard;
    writeln(TrumpCard.suit, ' ', TrumpCard.value);  
    writeln(card1.suit, ' ', card1.value);  
    writeln(card2.suit, ' ', card2.value);  
    writeln(CompareCards(card1, card2, TrumpCard.suit));
    writeln;
{$ENDIF}
{$IFDEF DECK}
    writeln(RemainingDeckSize(d));
    if not ok then
    begin
        writeln(ErrOutput, 'Failed to get trump suit');
        halt(1)
    end;
    writeln(trump);  
    writeln;
    for i := 1 to DeckSize do
    begin
        TryTakeTopCard(d, p, ok); 
        if not ok then
        begin
            writeln(ErrOutput, 'Failed to take top card');
            halt(1)
        end;
        writeln(p^.suit, ' ', p^.value);  
        writeln(RemainingDeckSize(d))
    end;
{$ENDIF}
{$IFDEF ENC}
    writeln('Trump suit: ', trump);
    writeln;
    h1p := @h1;
    h2p := @h2;
    InitHand(h1);
    InitHand(h2);
    InitRestockHands(h1, h2, d, ok);
    if not ok then
    begin
        writeln(ErrOutput, 'Could not fill hands on start!');
        halt(1)
    end;
    InitEncounter(e, h1p, h2p);
    IsAttackerTurn := true;
    while true do
    begin
        writeln('Hands:');
        for i := 1 to DeckSize do
        begin
            if h1[i] <> nil then
                write('P1: ', h1[i]^.value, ', ', h1[i]^.suit,  '; ');
            if h2[i] <> nil then
                write('P2: ', h2[i]^.value, ', ', h2[i]^.suit);
            if (h1[i] <> nil) or (h2[i] <> nil) then
                writeln
        end;
        writeln;
        writeln('Table:');
        for i := 1 to NormalHandSize do
        begin
            if e.AttackerTable[i] <> nil then
                write('P1: ', e.AttackerTable[i]^.value, ', ',
                    e.AttackerTable[i]^.suit,  '; ');
            if e.DefenderTable[i] <> nil then
                write('P2: ', e.DefenderTable[i]^.value, ', ',
                    e.DefenderTable[i]^.suit);
            if (e.AttackerTable[i] <> nil) or (e.DefenderTable[i] <> nil) then
                writeln
        end;
        writeln;
        if IsAttackerTurn then
            write('Attacker')
        else
            write('Defender');
        write(' turn, input card number: ');
        readln(inp);
        writeln;
        if IsAttackerTurn then
        begin
            if inp <= 0 then
            begin
                FinishEncounter(e);
                break
            end;
            for i := 1 to DeckSize do
                if h1[i] <> nil then
                    if inp > 1 then
                        inp := inp - 1
                    else
                    begin
                        AttackerTryPlayCard(e, h1[i], ok);
                        if not ok then
                            writeln('Can''t play this card as attacker')
                        else
                            IsAttackerTurn := not IsAttackerTurn;
                        break
                    end           
        end
        else
        begin
            if inp <= 0 then
            begin
                GiveUpEncounter(e);
                break
            end;
            for i := 1 to DeckSize do
                if h2[i] <> nil then
                    if inp > 1 then
                        inp := inp - 1
                    else
                    begin
                        DefenderTryPlayCard(e, h2[i], trump, ok);
                        if not ok then
                            writeln('Can''t play this card as defender')
                        else
                            IsAttackerTurn := not IsAttackerTurn;
                        break
                    end           
        end
    end;
    writeln('Final:');
    writeln('Hands:');
    for i := 1 to DeckSize do
    begin
        if h1[i] <> nil then
            write('P1: ', h1[i]^.value, ', ', h1[i]^.suit,  '; ');
        if h2[i] <> nil then
            write('P2: ', h2[i]^.value, ', ', h2[i]^.suit);
        if (h1[i] <> nil) or (h2[i] <> nil) then
            writeln
    end;
    writeln;
    writeln('Table:');
    for i := 1 to NormalHandSize do
    begin
        if e.AttackerTable[i] <> nil then
            write('P1: ', e.AttackerTable[i]^.value, ', ',
                e.AttackerTable[i]^.suit,  '; ');
        if e.DefenderTable[i] <> nil then
            write('P2: ', e.DefenderTable[i]^.value, ', ',
                e.DefenderTable[i]^.suit);
        if (e.AttackerTable[i] <> nil) or (e.DefenderTable[i] <> nil) then
            writeln
    end;
    RestockHand(h1, d);
    writeln;
    writeln('Restock1:');
    writeln('Hands:');
    for i := 1 to DeckSize do
    begin
        if h1[i] <> nil then
            write('P1: ', h1[i]^.value, ', ', h1[i]^.suit,  '; ');
        if h2[i] <> nil then
            write('P2: ', h2[i]^.value, ', ', h2[i]^.suit);
        if (h1[i] <> nil) or (h2[i] <> nil) then
            writeln
    end;
    writeln;
    writeln('Table:');
    for i := 1 to NormalHandSize do
    begin
        if e.AttackerTable[i] <> nil then
            write('P1: ', e.AttackerTable[i]^.value, ', ',
                e.AttackerTable[i]^.suit,  '; ');
        if e.DefenderTable[i] <> nil then
            write('P2: ', e.DefenderTable[i]^.value, ', ',
                e.DefenderTable[i]^.suit);
        if (e.AttackerTable[i] <> nil) or (e.DefenderTable[i] <> nil) then
            writeln
    end;
    RestockHand(h2, d);
    writeln;
    writeln('Restock2:');
    writeln('Hands:');
    for i := 1 to DeckSize do
    begin
        if h1[i] <> nil then
            write('P1: ', h1[i]^.value, ', ', h1[i]^.suit,  '; ');
        if h2[i] <> nil then
            write('P2: ', h2[i]^.value, ', ', h2[i]^.suit);
        if (h1[i] <> nil) or (h2[i] <> nil) then
            writeln
    end;
    writeln;
    writeln('Table:');
    for i := 1 to NormalHandSize do
    begin
        if e.AttackerTable[i] <> nil then
            write('P1: ', e.AttackerTable[i]^.value, ', ',
                e.AttackerTable[i]^.suit,  '; ');
        if e.DefenderTable[i] <> nil then
            write('P2: ', e.DefenderTable[i]^.value, ', ',
                e.DefenderTable[i]^.suit);
        if (e.AttackerTable[i] <> nil) or (e.DefenderTable[i] <> nil) then
            writeln
    end;
{$ENDIF}
end.
