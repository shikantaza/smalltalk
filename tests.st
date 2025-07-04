Smalltalk createClass: #UnitTestFramework

Smalltalk addClassVariable: #passedCases toClass: UnitTestFramework

Smalltalk addClassVariable: #failedCases toClass: UnitTestFramework

Smalltalk addClassMethod: #initialize toClass: UnitTestFramework withBody: [
  passedCases := 0. failedCases := 0
]

Smalltalk addClassMethod: #runTestCase:withID: toClass: UnitTestFramework withBody: [
  :blk :id |
  [
    Transcript show: 'Running test case '; show: id; cr.
    (blk value) ifTrue: [
          passedCases := passedCases + 1 ]
    ifFalse: [
      Transcript show: id; show: ' failed!'; cr.
      failedCases := failedCases + 1
    ]
  ] on: Exception do: [ :ex |
    Transcript show: id; show: ' failed!'; cr.
    failedCases := failedCases + 1
  ]
]

Smalltalk addClassMethod: #runNegativeTestCase:withID: toClass: UnitTestFramework withBody: [
  :blk :id |
  [
    Transcript show: 'Running test case '; show: id; cr.
    (blk value) ifTrue: [
      Transcript show: id; show: ' failed!'; cr.
      failedCases := failedCases + 1 ]
    ifFalse: [
      passedCases := passedCases + 1
    ]
  ] on: Exception do: [ :ex |
    Transcript show: id; show: ' failed!'; cr.
    failedCases := failedCases + 1
  ]
]

Smalltalk addClassMethod: #runExceptionTestCase:exception:withID: toClass: UnitTestFramework withBody: [
  :blk :ex :id |
  [
     Transcript show: 'Running test case '; show: id; cr.
     blk on: ex do: [ :ex1 | passedCases := passedCases + 1. ^ self ]
  ] on: Exception do: [ :ex1 |
    Transcript show: id; show: ' failed!'; cr.
    failedCases := failedCases + 1.
    ^ self
  ].
  Transcript show: id; show: ' failed!'; cr.
  failedCases := failedCases + 1
]

Smalltalk addClassMethod: #reportStatus toClass: UnitTestFramework withBody: [
  Transcript show: 'Passed test cases = '; show: passedCases; cr.
  Transcript show: 'Failed test cases = '; show: failedCases; cr
]

UnitTestFramework initialize

Smalltalk createGlobal: #classObj1 valued: (Smalltalk createClass: (Smalltalk genSym))

Smalltalk addClassMethod: #hello toClass: classObj1 withBody: [ ^ 42 ]

UnitTestFramework runTestCase: [ classObj1 hello = 42 ] withID: 1

Smalltalk addInstanceMethod: #method1 toClass: classObj1 withBody: [ ^ 33 ]

UnitTestFramework runTestCase: [ classObj1 new method1 = 33 ] withID: 2

Smalltalk createGlobal: #classObj2 valued: (Smalltalk createClass: (Smalltalk genSym) parentClass: classObj1)

Smalltalk addInstanceMethod: #method2 toClass: classObj2 withBody: [ ^42 ]

UnitTestFramework runTestCase: [ classObj2 new method2 = 42 ] withID: 3

Smalltalk createClass: #Fact

Smalltalk addClassMethod: #fact: toClass: Fact withBody:
  [ :n |
  (n = 0) ifTrue: [ ^ 1].
  ^ (n * (self fact: (n - 1)))]

UnitTestFramework runTestCase: [ (Fact fact: 10)  = 3628800 ] withID: 4

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addInstanceVariable: #x toClass: classObj1

Smalltalk addInstanceMethod: #x: toClass: classObj1 withBody: [ :v | x := v]

Smalltalk addInstanceMethod: #x toClass: classObj1 withBody: [ ^x ]

| m |
m := classObj1 new.
m x: 10.
UnitTestFramework runTestCase: [ ((m x) = 10)] withID: 5

classObj2 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassVariable: #x toClass: classObj2

Smalltalk addClassMethod: #x: toClass: classObj2 withBody: [ :v | x := v ]

Smalltalk addClassMethod: #x toClass: classObj2 withBody: [ ^x ]

classObj2 x: 42.
UnitTestFramework runTestCase: [((classObj2 x) = 42)] withID: 6

| x |
x := [1/0. 42] on: ZeroDivide do: [ :ex | ex return: 1].
UnitTestFramework runNegativeTestCase: [(x = 42)] withID: 7

| x |
x := 0.
[(x = 0) ifTrue: [ Exception new signal]] on: Exception do: [ :ex | x := 42].
UnitTestFramework runTestCase: [(x = 42)] withID: 8

| x |
x := 0.
[(x = 0) ifTrue: [ ZeroDivide new signal]] on: ZeroDivide do: [ :ex | x := 42].
UnitTestFramework runTestCase: [(x = 42)] withID: 9

| x |
x := 0.
[(x = 0) ifTrue: [ ZeroDivide new signal]] on: Error do: [ :ex | x := 42].
UnitTestFramework runTestCase: [(x = 42)] withID: 10

| x |
x := 0.
[(x = 0) ifTrue: [ ZeroDivide new signal]] on: Exception do: [ :ex | x := 42].
UnitTestFramework runTestCase: [(x = 42)] withID: 11

| x y |
x := 0.
[y := 10/x] on: ZeroDivide do: [ :ex | x := 2. ex retry].
UnitTestFramework runTestCase: [y = 5] withID: 12

| x y |
x := 0.
[y := 10/x] on: ZeroDivide do: [ :ex | x := 2. ex retryUsing: [y := 42]].
UnitTestFramework runTestCase: [y = 42] withID: 13

| x y |
x := 0.
[y := 1/x] on: ZeroDivide do: [ :ex | ex resume: 42].
UnitTestFramework runTestCase: [y = 42] withID: 14

| x y |
x := 0.
[[y := 1/x] on: ZeroDivide do: [:ex | ex pass]] on: Exception do: [ :ex | ex resume: 42].
UnitTestFramework runTestCase: [y = 42] withID: 15

| x y |
x := 0.
[[y := 1/x] on: ZeroDivide do: [:ex | ex outer. y := 33 ]] on: Exception do: [ :ex | ex resume].
UnitTestFramework runTestCase: [y = 33] withID: 16

| x y |
x := 0.
[[y := 1/x] on: ZeroDivide do: [ :ex | ex resignalAs: (Error new)]] on: Exception do: [ :ex | y := 42].
UnitTestFramework runTestCase: [y = 42] withID: 17

| x y |
x := 0.
[[y := 1/x] on: ZeroDivide do: [ :ex | ex resignalAs: (Exception new)]] on: Exception do: [ :ex | y := 42].
UnitTestFramework runTestCase: [y = 42] withID: 18

| x y z|
x := 0.
[[y := 1/x] on: ZeroDivide do: [ :ex | y := 33. ex return ]] ensure: [ Transcript show: 'ensured'; cr. z := 42 ].
UnitTestFramework runTestCase: [(y = 33) & (z = 42)] withID: 19

| x y z|
x := 0.
[[y := 1/x] ifCurtailed: [ z := 42]] on: ZeroDivide do: [:ex | ex resume: 33].
UnitTestFramework runTestCase: [(y = 33) & (z = nil)] withID: 20

Smalltalk createClass: #ArgTest

Smalltalk addClassMethod: #test1: toClass: ArgTest withBody: [:x | ^x ]

UnitTestFramework runTestCase: [(ArgTest test1: 42) = 42] withID: 21

Smalltalk addClassMethod: #p1:p2: toClass: ArgTest withBody: [:x1 :x2 | ^ (x1 + x2) ]

UnitTestFramework runTestCase: [(ArgTest p1: 1 p2: 2) = 3] withID: 22

Smalltalk addClassMethod: #p1:p2:p3: toClass: ArgTest withBody: [:x1 :x2 :x3 | ^ (x1 + x2 + x3)]

UnitTestFramework runTestCase: [(ArgTest p1: 1 p2: 2 p3: 3) = 6] withID: 23

Smalltalk addClassMethod: #p1:p2:p3:p4: toClass: ArgTest withBody: [:x1 :x2 :x3 :x4 | ^ (x1 + x2 + x3 + x4)]

UnitTestFramework runTestCase: [(ArgTest p1: 1 p2: 2 p3: 3 p4: 4) = 10] withID: 24

UnitTestFramework runExceptionTestCase: [42 hello] exception: MessageNotUnderstood withID: 25

UnitTestFramework runExceptionTestCase: [1/0] exception: ZeroDivide withID: 26

Smalltalk createGlobal: #classObj3 valued: (Smalltalk createClass: (Smalltalk genSym))

Smalltalk addClassMethod: #m3 toClass: classObj3 withBody: [ ^ (1/0)]

classObj2 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m2 toClass: classObj2 withBody: [ [ ^(classObj3 m3)] ifCurtailed: [ Transcript show: 'curtailed'; cr]]

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m1 toClass: classObj1 withBody: [ [^(classObj2 m2)] on: ZeroDivide do: [ :ex | ex resume]]

Smalltalk addClassMethod: #m12 toClass: classObj1 withBody: [ [ ^(classObj2 m2)] on: ZeroDivide do: [ :ex | ex resume: 42]]

| x |
x := classObj1 m12. Transcript show: x; cr.
UnitTestFramework runTestCase: [x = 42] withID: 27

classObj1 m1

Smalltalk createGlobal: #g1 valued: 0

Smalltalk createGlobal: #g2 valued: 0

classObj3 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m6 toClass: classObj3 withBody: [g1 := 10 / g2. ^ g1]

classObj2 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m5 toClass: classObj2 withBody: [ [^(classObj3 m6)] ifCurtailed: [ Transcript show: 'curtailed'; cr]]

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m4 toClass: classObj1 withBody: [ [^(classObj2 m5)] on: ZeroDivide do: [ :ex | g2 := 5. ex retry]]

classObj1 m4

UnitTestFramework runTestCase: [g1 = 2] withID: 28

Smalltalk addClassMethod: #m41 toClass: classObj1 withBody: [ [^(classObj2 m5)] on: ZeroDivide do: [ :ex | ex retryUsing: [g2 := 5. ^(classObj2 m5)]]]

g2 := 0

classObj1 m41

UnitTestFramework runTestCase: [g1 = 2] withID: 29

Smalltalk addClassMethod: #m42 toClass: classObj1 withBody: [ [^(classObj2 m5)] on: ZeroDivide do: [ :ex | ex return]]

g1 := 0

g2 := 0

classObj1 m42

UnitTestFramework runTestCase: [g1 = 0] withID: 30

Smalltalk addClassMethod: #m43 toClass: classObj1 withBody: [ [g1 := classObj2 m5] on: ZeroDivide do: [ :ex | ex resume: 42]]

g1 := 0

g2 := 0

classObj1 m43

UnitTestFramework runTestCase: [g1 = 42] withID: 31

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m7 toClass: classObj1 withBody: [[ ^ 42] ifCurtailed: [g1 := 33]]

classObj1 m7

UnitTestFramework runTestCase: [g1 = 33] withID: 32

| x y|
[[x := 1/0. y := x + 32] on: ZeroDivide do: [ :ex | ex resume: 10]] ifCurtailed: [ y := 14].
UnitTestFramework runTestCase: [(x = 10) & (y = 42)] withID: 33

classObj3 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m10 toClass: classObj3 withBody: [^(1/0)]

classObj2 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m9 toClass: classObj2 withBody: [ [^(classObj3 m10)] ensure: [ Transcript show: 'ensure'; cr]]

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m8 toClass: classObj1 withBody: [ [^(classObj2 m9)] on: ZeroDivide do: [ :ex | ex resume]]

Smalltalk addClassMethod: #m82 toClass: classObj1 withBody: [ [^(classObj2 m9)] on: ZeroDivide do: [ :ex | ex resume: 42]]

| x |
x := classObj1 m82.
UnitTestFramework runTestCase: [x = 42] withID: 34

classObj1 m8

g1 := 0

g2 := 0

classObj3 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m13 toClass: classObj3 withBody: [g1 := 10 / g2. ^ g1]

classObj2 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m12 toClass: classObj2 withBody: [ [^(classObj3 m13)] ensure: [ Transcript show: 'ensure'; cr]]

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m11 toClass: classObj1 withBody: [ [^(classObj2 m12)] on: ZeroDivide do: [ :ex | g2 := 5. ex retry]]

classObj1 m11

UnitTestFramework runTestCase: [g1 = 2] withID: 35

Smalltalk addClassMethod: #m111 toClass: classObj1 withBody: [ [^(classObj2 m12)] on: ZeroDivide do: [ :ex | ex retryUsing: [g2 := 5. classObj2 m12]]]

g2 := 0

classObj1 m111

UnitTestFramework runTestCase: [g1 = 2] withID: 36

Smalltalk addClassMethod: #m112 toClass: classObj1 withBody: [ [^(classObj2 m12)] on: ZeroDivide do: [ :ex | ex return]]

g1 := 0

g2 := 0

classObj1 m112

UnitTestFramework runTestCase: [g1 = 0] withID: 37

Smalltalk addClassMethod: #m113 toClass: classObj1 withBody: [ [g1 := classObj2 m12] on: ZeroDivide do: [ :ex | ex resume: 42]]

g1 := 0

g2 := 0

classObj1 m113

UnitTestFramework runTestCase: [g1 = 42] withID: 38

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addInstanceVariable: #x toClass: classObj1

Smalltalk addInstanceMethod: #set toClass: classObj1 withBody: [ x := 33]

Smalltalk addInstanceMethod: #get toClass: classObj1 withBody: [ ^x ]

classObj2 := Smalltalk createClass: (Smalltalk genSym) parentClass: classObj1

Smalltalk addInstanceMethod: #set toClass: classObj2 withBody: [ x := 42]

Smalltalk addInstanceMethod: #test toClass: classObj2 withBody: [ super set]

|obj|
obj := classObj2 new. obj test.
UnitTestFramework runTestCase: [(obj get) = 33] withID: 39

|obj|
obj := classObj2 new. obj set.
UnitTestFramework runTestCase: [(obj get) = 42] withID: 40

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addInstanceVariable: #x toClass: classObj1

Smalltalk addInstanceMethod: #initialize toClass: classObj1 withBody: [ x := 42]

Smalltalk addInstanceMethod: #x toClass: classObj1 withBody: [ ^ x ]

| a | a := classObj1 new. UnitTestFramework runTestCase: [ (a x) = 42 ] withID: 41

classObj2 := Smalltalk createClass: (Smalltalk genSym) parentClass: classObj1

Smalltalk addInstanceVariable: #y toClass: classObj2

Smalltalk addInstanceMethod: #initialize toClass: classObj2 withBody: [ y := 33]

Smalltalk addInstanceMethod: #y toClass: classObj2 withBody: [ ^ y ]

| a | a := classObj2 new. UnitTestFramework runTestCase: [((a x) = 42 & ((a y) = 33))] withID: 42

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addInstanceVariable: #x toClass: classObj1

Smalltalk addInstanceMethod: #x toClass: classObj1 withBody: [ ^ x ]

Smalltalk addInstanceMethod: #initialize toClass: classObj1 withBody: [ [1/0] on: ZeroDivide do: [ :ex | ex resume: 42]. x := 33 ]

| a | a := classObj1 new.
UnitTestFramework runTestCase: [(a x) = 33] withID: 43

UnitTestFramework runExceptionTestCase: [1 + false] exception: InvalidArgument withID: 44

UnitTestFramework runExceptionTestCase: [1 - false] exception: InvalidArgument withID: 45

UnitTestFramework runExceptionTestCase: [1 * false] exception: InvalidArgument withID: 46

UnitTestFramework runExceptionTestCase: [1 / false] exception: InvalidArgument withID: 47

UnitTestFramework runTestCase: [1 = 1] withID: 48

UnitTestFramework runNegativeTestCase: [1 = Transcript] withID: 49

UnitTestFramework runExceptionTestCase: [Smalltalk createClass: 1] exception: InvalidArgument withID: 50

UnitTestFramework runExceptionTestCase: [Smalltalk createClass: #SomeClass parentClass: 1] exception: InvalidArgument withID: 51

classObj1 := Smalltalk createClass: (Smalltalk genSym)

UnitTestFramework runExceptionTestCase: [Smalltalk addInstanceVariable: 1 toClass: classObj1] exception: InvalidArgument withID: 52

UnitTestFramework runExceptionTestCase: [Smalltalk addInstanceVariable: #x toClass: 1] exception: InvalidArgument withID: 53

UnitTestFramework runExceptionTestCase: [Smalltalk addClassVariable: 1 toClass: classObj1] exception: InvalidArgument withID: 54

UnitTestFramework runExceptionTestCase: [Smalltalk addClassVariable: #x toClass: 1] exception: InvalidArgument withID: 55

UnitTestFramework runExceptionTestCase: [Smalltalk createGlobal: 1 valued: 10] exception: InvalidArgument withID: 56

UnitTestFramework runExceptionTestCase: [Smalltalk createGlobal: 1] exception: InvalidArgument withID: 57

| a | a := Array new: 3.
UnitTestFramework runTestCase: [((a at: 1) = nil) & ((a at: 2) = nil) & ((a at: 3) = nil) ] withID: 58

| a | a := Array new: 3. a at: 1 put: 42.
UnitTestFramework runTestCase: [((a at: 1) = 42) & ((a at: 2) = nil) & ((a at: 3) = nil) ] withID: 59

| a | a := Array new: 3.
UnitTestFramework runTestCase: [((a size) = 3) ] withID: 60

| a | a := Array new: 3.
UnitTestFramework runExceptionTestCase: [a at: 4 put: 42] exception: IndexOutofBounds withID: 61

| a | a := Array new: 3.
UnitTestFramework runExceptionTestCase: [a at: #a put: 42] exception: InvalidArgument withID: 62

| a | a := Array new: 3.
UnitTestFramework runExceptionTestCase: [a at: 4] exception: IndexOutofBounds withID: 63

| a | a := Array new: 3.
UnitTestFramework runExceptionTestCase: [a at: #a] exception: InvalidArgument withID: 64

UnitTestFramework runExceptionTestCase: [Array new: #a] exception: InvalidArgument withID: 65

UnitTestFramework runExceptionTestCase: [Array new: -1] exception: InvalidArgument withID: 66

UnitTestFramework runExceptionTestCase: [Array new: 0] exception: InvalidArgument withID: 67

| a | a := Array new: 3. a at: 1 put: 1. a at: 2 put: 2. a at: 3 put: 3.
UnitTestFramework runExceptionTestCase: [a do: 1] exception: InvalidArgument withID: 68

| a x |
x := 0.
a := Array new: 3. a at: 1 put: 1. a at: 2 put: 2. a at: 3 put: 3.
a do: [ :v | x := x + v].
UnitTestFramework runTestCase: [ x = 6] withID: 69

Smalltalk createClass: #MutualRecurTest

Smalltalk addClassMethod: #isOdd: toClass: MutualRecurTest withBody: [
  :v |
  (v < 0) ifTrue: [Error new signal].
  (v = 0) ifTrue: [
    ^false
  ]
  ifFalse: [
    ^self isEven: (v - 1)
  ]
]

Smalltalk addClassMethod: #isEven: toClass: MutualRecurTest withBody: [
  :v |
  (v < 0) ifTrue: [Error new signal].
  (v = 0) ifTrue: [
    ^true
  ]
  ifFalse: [
    ^self isOdd: (v - 1)
  ]
]

UnitTestFramework runTestCase: [MutualRecurTest isOdd: 1] withID: 70

UnitTestFramework runTestCase: [MutualRecurTest isOdd: 7] withID: 71

UnitTestFramework runNegativeTestCase: [MutualRecurTest isOdd: 0] withID: 72

UnitTestFramework runNegativeTestCase: [MutualRecurTest isOdd: 8] withID: 73

UnitTestFramework runTestCase: [MutualRecurTest isEven: 0] withID: 74

UnitTestFramework runTestCase: [MutualRecurTest isEven: 8] withID: 75

UnitTestFramework runNegativeTestCase: [MutualRecurTest isEven: 1] withID: 76

UnitTestFramework runNegativeTestCase: [MutualRecurTest isEven: 7] withID: 77

UnitTestFramework runExceptionTestCase: [MutualRecurTest isOdd: -42] exception: Error withID: 78

UnitTestFramework runExceptionTestCase: [MutualRecurTest isEven: -42] exception: Error withID: 79

classObj2 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m2 toClass: classObj2 withBody: [
  [ Transcript show: 'executing m2'; cr. g2 := 33 ]
      ensure: [ Transcript show: 'ensuring'; cr.
                1 / 0.
                Transcript show: 'resuming'; cr.
                g1 := 42 ]
]

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m1 toClass: classObj1 withBody: [ [^ classObj2 m2] on: ZeroDivide do: [ :ex | ex resume: 10]]

classObj1 m1

UnitTestFramework runTestCase: [ (g1 = 42) & (g2 = 33)] withID: 80

Smalltalk addClassMethod: #m11 toClass: classObj1 withBody: [ ^ classObj2 m2 ]

g1 := nil

UnitTestFramework runExceptionTestCase: [classObj1 m11] exception: ZeroDivide withID: 81

UnitTestFramework runTestCase: [g1 = nil] withID: 82

Smalltalk createGlobal: #g3

classObj3 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m15 toClass: classObj3 withBody: [ [classObj2 m2] ifCurtailed: [ g3 := 42]]

Smalltalk addClassMethod: #m12 toClass: classObj1 withBody: [ ^ classObj3 m15]

UnitTestFramework runExceptionTestCase: [classObj1 m12] exception: ZeroDivide withID: 83

UnitTestFramework runTestCase: [(g1 = nil) & (g3 = 42)] withID: 84

Smalltalk addClassMethod: #m13 toClass: classObj1 withBody: [ [ ^ classObj3 m15] on: ZeroDivide do: [:ex | ex resume: 10 ]]

classObj1 m13

UnitTestFramework runTestCase: [(g1 = 42) & (g3 = 42)] withID: 85

| x y z|
[x := 1/0] on: ZeroDivide do: [ :ex | [y := 1/0] on: ZeroDivide do: [ :ex1 | ex1 resume: 42 ]. z := 33. ex resume: 10 ].
UnitTestFramework runTestCase: [(x = 10) & (y = 42) & (z = 33)] withID: 86

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m1 toClass: classObj1 withBody: [
  [g1 := 1/0] ensure: [ [g2 := 1/0] on: ZeroDivide do: [ :ex1 | ex1 resume: 42 ]]
]

UnitTestFramework runExceptionTestCase: [classObj1 m1] exception: ZeroDivide withID: 87

UnitTestFramework runTestCase: [g2 = 42] withID: 88

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addClassMethod: #m1 toClass: classObj1 withBody: [
  [^1] ensure: [ Transcript show: 'ensure'; cr ].
  ^ 2
]

UnitTestFramework runTestCase: [(classObj1 m1) = 1] withID: 89

Smalltalk createGlobal: #s valued: (Stack new)

UnitTestFramework runTestCase: [s isEmpty] withID: 90

s push: 42

UnitTestFramework runTestCase: [(s top) = 42] withID: 91

UnitTestFramework runNegativeTestCase: [s isEmpty] withID: 92

UnitTestFramework runTestCase: [(s pop) = 42] withID: 93

UnitTestFramework runTestCase: [s isEmpty] withID: 94

classObj1 := Smalltalk createClass: (Smalltalk genSym)

Smalltalk addInstanceMethod: #m1 toClass: classObj1 withBody: [ ^ 42]

UnitTestFramework runTestCase: [(classObj1 new m1) = 42] withID: 95

Smalltalk addClassMethod: #m2 toClass: classObj1 withBody: [ ^ 33]

UnitTestFramework runTestCase: [(classObj1 m2) = 33] withID: 96

UnitTestFramework runTestCase: [('Hello world!' size) = 12] withID: 97

UnitTestFramework runTestCase: [('Hello world!' isEmpty) = false] withID: 98

UnitTestFramework runTestCase: [('Hello world!' notEmpty) = true] withID: 99

UnitTestFramework runTestCase: [('' isEmpty) = true] withID: 100

UnitTestFramework runTestCase: [('' notEmpty) = false] withID: 101

|sum|
sum := 0.
'Hello world!' do: [ :c | sum := sum + 1].
UnitTestFramework runTestCase: [ sum = 12 ] withID: 102

|sum1 sum2|
sum1 := 0.
sum2 := 0.
'Hello world!' do: [ :c | sum1 := sum1 + 1] separatedBy: [ sum2 := sum2 + 1].
UnitTestFramework runTestCase: [ (sum1 = 12) & (sum2 = 11) ] withID: 103

UnitTestFramework runTestCase: [ ('Hello world!' select: [ :c | c = $o ]) = 'oo'] withID: 104

UnitTestFramework runTestCase: [ ('Hello world!' reject: [ :c | c = $o ]) = 'Hell wrld!'] withID: 105

UnitTestFramework runTestCase: [ ('Hello world!' occurrencesOf: $o) = 2 ] withID: 106

UnitTestFramework runTestCase: [ 'Hello world!' includes: $o ] withID: 107

UnitTestFramework runNegativeTestCase: [ 'Hello world!' includes: $x ] withID: 108

|x|
x := 0.
UnitTestFramework runTestCase: [ (('Hello world!' detect: [ :c | c = $o] ifNone: [ x := 42]) = $o) & (x = 0)] withID: 109

UnitTestFramework runTestCase: [ ('Hello world!' detect: [ :c | c = $x] ifNone: [ 42 ]) = 42 ] withID: 110

UnitTestFramework runTestCase: [ ('Hello world!' detect: [ :c | c = $o]) = $o ] withID: 111

UnitTestFramework runTestCase: [ ('Hello world!' detect: [ :c | c = $x]) = nil ] withID: 112

UnitTestFramework runTestCase: [
  ('Hello world!' collect: [ :c |
    (c = $o) ifTrue: [ $x ] ifFalse: [ c ]
  ]) = 'Hellx wxrld!'
] withID: 113

UnitTestFramework runTestCase: [ (('Hello world!' substringsForSeparators: ' ' ) size) = 2 ] withID: 114

UnitTestFramework runTestCase: [ (('Hello world!' substringsForSeparators: ' ' ) at: 1) = 'Hello' ] withID: 115

UnitTestFramework runTestCase: [ (('Hello world!' substringsForSeparators: ' ' ) at: 2) = 'world!' ] withID: 116

Smalltalk createGlobal: #oc valued: (OrderedCollection new)

UnitTestFramework runTestCase: [(oc size) = 0] withID: 117

oc add: 42

UnitTestFramework runTestCase: [(oc size) = 1] withID: 118

UnitTestFramework runTestCase: [(oc at: 1) = 42] withID: 119

oc add: 33

UnitTestFramework runTestCase: [ (oc size) = 2 ] withID: 120

UnitTestFramework runTestCase: [(oc at: 1) = 42] withID: 121

UnitTestFramework runTestCase: [(oc at: 2) = 33] withID: 122

UnitTestFramework runExceptionTestCase: [oc at: 3] exception: IndexOutofBounds withID: 123

UnitTestFramework runExceptionTestCase: [oc at: Transcript] exception: InvalidArgument withID: 124

oc removeLast

UnitTestFramework runTestCase: [(oc size) = 1] withID: 125

Smalltalk addClassMethod: #p1:p2:p3:p4:p5: toClass: ArgTest withBody: [:x1 :x2 :x3 :x4 :x5 | ^ (x1 + x2 + x3 + x4 + x5)]

UnitTestFramework runTestCase: [(ArgTest p1: 1 p2: 2 p3: 3 p4: 4 p5: 5) = 15] withID: 126

UnitTestFramework reportStatus
