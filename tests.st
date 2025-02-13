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

Smalltalk createClass: #TestIfCurtailed

Smalltalk addClassMethod: #test: toClass: TestIfCurtailed withBody:
  [ :x |
  [ (x = 0) ifTrue: [^ 0].
    Transcript show: 'not zero'; cr. ^ 42]
  ifCurtailed:
    [Transcript show: 'curtailed'; cr]]

| x |
x := TestIfCurtailed test: 0.
(x = 0) ifTrue:
  [ passedCases := passedCases + 1]
ifFalse:
  [ failedCases := failedCases + 1]

| x |
x := TestIfCurtailed test: 1.
(x = 42) ifTrue:
  [ passedCases := passedCases + 1]
ifFalse:
  [ failedCases := failedCases + 1]

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

