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

Smalltalk addClassMethod: #p1:p2:p3:p4:p5: toClass: ArgTest withBody: [:x1 :x2 :x3 :x4 :x5 | x1 + x2 + x3 + x4 + x5]

((ArgTest p1: 1 p2: 2 p3: 3 p4: 4 p5: 5) = 15) ifTrue: [passedCases := passedCases + 1] ifFalse: [failedCases := failedCases + 1]

Transcript show: 'Passed test cases = '. passedCases

Transcript show: 'Failed test cases = '. failedCases