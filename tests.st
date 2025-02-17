Smalltalk createGlobal: #passedCases valued: 0

Smalltalk createGlobal: #failedCases valued: 0

Smalltalk createClass: #MyClass

Smalltalk addClassMethod: #hello toClass: MyClass withBody: [ 42 ]

(MyClass hello = 42) ifTrue:
  [passedCases := passedCases + 1]
ifFalse:
  [failedCases := failedCases + 1]

Smalltalk createClass: #MyClass1

Smalltalk addInstanceMethod: #method1 toClass: MyClass1 withBody: [ 33 ]

(MyClass1 new method1 = 33) ifTrue:
  [passedCases := passedCases + 1]
ifFalse:
  [failedCases := failedCases + 1]

Smalltalk createClass: #MyClass2 parentClass: MyClass1

Smalltalk addInstanceMethod: #method2 toClass: MyClass2 withBody: [ 42 ]

(MyClass2 new method2 = 42) ifTrue:
  [passedCases := passedCases + 1]
ifFalse:
  [failedCases := failedCases + 1]

Smalltalk createClass: #Fact

Smalltalk addClassMethod: #fact: toClass: Fact withBody:
  [ :n |
  (n = 0) ifTrue: [ ^ 1].
  ^ (n * (self fact: (n - 1)))]

((Fact fact: 10)  = 3628800) ifTrue:
  [passedCases := passedCases + 1]
ifFalse:
  [failedCases := failedCases + 1]

Smalltalk createClass: #MyClass3

Smalltalk addInstanceVariable: #x toClass: MyClass3

Smalltalk addInstanceMethod: #x: toClass: MyClass3 withBody: [ :v | x := v]

Smalltalk addInstanceMethod: #x toClass: MyClass3 withBody: [ x ]

| m |
m := MyClass3 new.
m x: 10.
((m x) = 10) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk createClass: #MyClass4

Smalltalk addClassVariable: #x toClass: MyClass4

Smalltalk addClassMethod: #x: toClass: MyClass4 withBody: [ :v | x := v ]

Smalltalk addClassMethod: #x toClass: MyClass4 withBody: [ x ]

MyClass4 x: 42.
((MyClass4 x) = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

(([1/0. 42] on: ZeroDivide do: [ :ex | passedCases := passedCases + 1. ex return: 1]) = 42) ifTrue: [ failedCases := failedCases + 1]

| x |
x := 0.
[(x = 0) ifTrue: [ Exception new signal]] on: Exception do: [ :ex | x := 42].
(x = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x |
x := 0.
[(x = 0) ifTrue: [ ZeroDivide new signal]] on: ZeroDivide do: [ :ex | x := 42].
(x = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x |
x := 0.
[(x = 0) ifTrue: [ ZeroDivide new signal]] on: Error do: [ :ex | x := 42].
(x = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x |
x := 0.
[(x = 0) ifTrue: [ ZeroDivide new signal]] on: Exception do: [ :ex | x := 42].
(x = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y |
x := 0.
[y := 10/x] on: ZeroDivide do: [ :ex | x := 2. ex retry].
(y = 5) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y |
x := 0.
[y := 10/x] on: ZeroDivide do: [ :ex | x := 2. ex retryUsing: [y := 42]].
(y = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y |
x := 0.
[y := 1/x] on: ZeroDivide do: [ :ex | ex resume: 42].
(y = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y |
x := 0.
[[y := 1/x] on: ZeroDivide do: [:ex | ex pass]] on: Exception do: [ :ex | ex resume: 42].
(y = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y |
x := 0.
[[y := 1/x] on: ZeroDivide do: [:ex | ex outer. y := 33 ]] on: Exception do: [ :ex | ex resume].
(y = 33) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y |
x := 0.
[[y := 1/x] on: ZeroDivide do: [ :ex | ex resignalAs: (Error new)]] on: Exception do: [ :ex | y := 42].
(y = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y |
x := 0.
[[y := 1/x] on: ZeroDivide do: [ :ex | ex resignalAs: (Exception new)]] on: Exception do: [ :ex | y := 42].
(y = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y z|
x := 0.
[[y := 1/x] on: ZeroDivide do: [ :ex | y := 33]] ensure: [ z := 42].
((y = 33) & (z = 42)) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y z|
x := 0.
[[y := 1/x] ifCurtailed: [ z := 42]] on: ZeroDivide do: [:ex | ex resume: 33].
((y = 33) & (z = 42)) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk createClass: #ArgTest

Smalltalk addClassMethod: #test1: toClass: ArgTest withBody: [:x | x ]

((ArgTest test1: 42) = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk addClassMethod: #p1:p2: toClass: ArgTest withBody: [:x1 :x2 | x1 + x2 ]

((ArgTest p1: 1 p2: 2) = 3) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk addClassMethod: #p1:p2:p3: toClass: ArgTest withBody: [:x1 :x2 :x3 | x1 + x2 + x3]

((ArgTest p1: 1 p2: 2 p3: 3) = 6) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk addClassMethod: #p1:p2:p3:p4: toClass: ArgTest withBody: [:x1 :x2 :x3 :x4 | x1 + x2 + x3 + x4]

((ArgTest p1: 1 p2: 2 p3: 3 p4: 4) = 10) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x |
[42 hello] on: MessageNotUnderstood do: [ :ex | x := 42].
(x = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk createClass: #Class3

Smalltalk addClassMethod: #m3 toClass: Class3 withBody: [1/0]

Smalltalk createClass: #Class2

Smalltalk addClassMethod: #m2 toClass: Class2 withBody: [ [Class3 m3] ifCurtailed: [ Transcript show: 'curtailed'; cr]]

Smalltalk createClass: #Class1

Smalltalk addClassMethod: #m1 toClass: Class1 withBody: [ [Class2 m2] on: ZeroDivide do: [ :ex | ex resume]]

Smalltalk addClassMethod: #m12 toClass: Class1 withBody: [ [Class2 m2] on: ZeroDivide do: [ :ex | ex resume: 42]]

| x |
x := Class1 m12.
(x = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Class1 m1

Smalltalk createGlobal: #g1 valued: 0

Smalltalk createGlobal: #g2 valued: 0

Smalltalk createClass: #Class6

Smalltalk addClassMethod: #m6 toClass: Class6 withBody: [g1 := 10 / g2]

Smalltalk createClass: #Class5

Smalltalk addClassMethod: #m5 toClass: Class5 withBody: [ [Class6 m6] ifCurtailed: [ Transcript show: 'curtailed'; cr]]

Smalltalk createClass: #Class4

Smalltalk addClassMethod: #m4 toClass: Class4 withBody: [ [Class5 m5] on: ZeroDivide do: [ :ex | g2 := 5. ex retry]]

Class4 m4

(g1 = 2) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk addClassMethod: #m41 toClass: Class4 withBody: [ [Class5 m5] on: ZeroDivide do: [ :ex | ex retryUsing: [g2 := 5. Class5 m5]]]

g2 := 0

Class4 m41

(g1 = 2) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk addClassMethod: #m42 toClass: Class4 withBody: [ [Class5 m5] on: ZeroDivide do: [ :ex | ex return]]

g1 := 0

g2 := 0

Class4 m42

(g1 = 0) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk addClassMethod: #m43 toClass: Class4 withBody: [ [g1 := Class5 m5] on: ZeroDivide do: [ :ex | ex return: 42]]

g1 := 0

g2 := 0

Class4 m43

(g1 = 42) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Smalltalk createClass: #Class7

Smalltalk addClassMethod: #m7 toClass: Class7 withBody: [[ ^ 42] ifCurtailed: [g1 := 33]]

Class7 m7

(g1 = 33) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

| x y|
[[x := 1/0. y := x + 32] on: ZeroDivide do: [ :ex | ex resume: 10]] ifCurtailed: [ y := 14].
((x = 10) & (y = 42)) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Transcript show: 'Passed test cases = '. passedCases

Transcript show: 'Failed test cases = '. failedCases
