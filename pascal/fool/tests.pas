program tests; { tests.pas }
uses deck, card;
var
{$IFDEF CARD}
    card1, card2, TrumpCard: PlayingCard;
{$ENDIF}
{$IFDEF DECK}
    d: CardDeck;
    p: CardPtr;
    trump: CardSuit;
    ok: boolean;
    i: integer;
{$ENDIF}
begin
    randomize;
{$IFDEF CARD}
    TrumpCard := RandomCard;
    card1 := RandomCard;
    card2 := RandomCard;
    writeln(TrumpCard.suit, ' ', TrumpCard.value);  
    writeln(card1.suit, ' ', card1.value);  
    writeln(card2.suit, ' ', card2.value);  
    writeln(CompareCards(card1, card2, TrumpCard.suit));
{$ENDIF}
{$IFDEF DECK}
{$IFDEF CARD}
    writeln;
{$ENDIF}
    GenerateDeck(d);
    writeln(RemainingDeckSize(d));
    TryGetTrumpSuit(d, trump, ok);
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
end.
