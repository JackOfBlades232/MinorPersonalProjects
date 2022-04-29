program MDSCalculator; { mds.pas }
const
    dim = 4;
    decimals = 2;
    FieldWidth = 7;
type
    vector = array [1..dim] of real;
    matrix = array [1..dim] of vector;

procedure PrintVector(v: vector);
var
    i: integer;
begin
    for i := 1 to dim do
        write(v[i]:FieldWidth:decimals);
    writeln
end;

procedure ReadMatrix(var m: matrix);
var
    i, j: integer;
begin
    for i := 1 to dim do
    begin
        for j := 1 to dim do
            read(m[i][j]);
        readln
    end
end;

procedure PrintMatrix(m: matrix);
var
    i, j: integer;
begin
    for i := 1 to dim do
    begin
        for j := 1 to dim do
                write(m[i][j]:FieldWidth:decimals);
        writeln
    end
end;

function ExtractColumn(m: matrix; ind: integer): vector;
var
    i: integer;
begin
    for i := 1 to dim do
        ExtractColumn[i] := m[i][ind]
end;

function CalculateScalarProduct(a1, a2: vector): real;
var
    i: integer;
begin
    CalculateScalarProduct := 0;
    for i := 1 to dim do
        CalculateScalarProduct := CalculateScalarProduct + a1[i] * a2[i]
end;

function CalculateNegative(m: matrix): matrix;
var
    i, j: integer;
begin
    for i := 1 to dim do
        for j := 1 to dim do
            CalculateNegative[i][j] := -m[i][j]
end;

function AddMatrices(m1, m2: matrix): matrix;
var
    i, j: integer;
begin
    for i := 1 to dim do
        for j := 1 to dim do
            AddMatrices[i][j] := m1[i][j] + m2[i][j]
end;

function SubtractMatrices(m1, m2: matrix): matrix;
begin
    SubtractMatrices := AddMatrices(m1, CalculateNegative(m2))
end;

function MultiplyMatrices(m1, m2: matrix): matrix;
var
    i, j: integer;
begin
    for i := 1 to dim do
        for j := 1 to dim do
            MultiplyMatrices[i][j] := 
                CalculateScalarProduct(m1[i], ExtractColumn(m2, j))
end;

function CalculateNormSquare(m: matrix): real;
var
    i, j: integer;
begin
    CalculateNormSquare := 0;
    for i := 1 to dim do
        for j := 1 to dim do
            CalculateNormSquare := 
                CalculateNormSquare + (m[i][j] * m[i][j])
end;

function CalculateK(m: matrix): matrix;
var
    i, j: integer;
    divisor: real;
begin
    divisor := 2.0 * dim;
    for i := 1 to dim do
        for j := 1 to dim do
            CalculateK[i][j] := -m[i][j] * m[i][j] / divisor
end;

function CalculateIdentityMatrix: matrix;
var
    i, j: integer;
begin
    for i := 1 to dim do
        for j := 1 to dim do
            if i = j then
                CalculateIdentityMatrix[i][j] := 1
            else
                CalculateIdentityMatrix[i][j] := 0
end;

function CalculateDoubleCenteringMatrix: matrix;
var
    i, j: integer;
    divisor: real;
begin
    divisor := dim;
    for i := 1 to dim do
        for j := 1 to dim do
            CalculateDoubleCenteringMatrix[i][j] := -1.0 / divisor;
    CalculateDoubleCenteringMatrix := 
        AddMatrices(CalculateDoubleCenteringMatrix, 
            CalculateIdentityMatrix)
end;

function CalculateGramMatrix(m: matrix): matrix;
var
    i, j: integer;
begin
    for i := 1 to dim do
        for j := 1 to dim do
            CalculateGramMatrix[i][j] := 
                CalculateScalarProduct(m[i], m[j])
end;

function CalculateT(ds, k: matrix): matrix;
begin
    CalculateT := MultiplyMatrices(MultiplyMatrices(ds, k), ds)
end;

function CalculateStrain(t, y: matrix): real;
begin
    CalculateStrain := 
        CalculateNormSquare(SubtractMatrices(t, CalculateGramMatrix(y)))
end;

var
    m, k, ds, t: matrix;
begin
    ds := CalculateDoubleCenteringMatrix;
    ReadMatrix(m);
    k := CalculateK(m);
    PrintMatrix(m);
    writeln;
    PrintMatrix(k);
    writeln;
    PrintMatrix(CalculateT(ds, k));
    writeln;
    writeln(CalculateStrain(CalculateT(ds, k), m))
end.
