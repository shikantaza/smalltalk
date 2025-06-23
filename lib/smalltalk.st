Smalltalk addClassMethod: #addInstanceVariables:toClass: toClass: Smalltalk  withBody: [
  :lst :cls |
  lst do: [ :v | self addInstanceVariable: v toClass: cls ]
]

Smalltalk addClassMethod: #addClassVariables:toClass: toClass: Smalltalk  withBody: [
  :lst :cls |
  lst do: [ :v | self addClassVariable: v toClass: cls ]
]

Smalltalk createClassPreInitialize: #Error parentClass: Exception

Smalltalk createClassPreInitialize: #MessageNotUnderstood parentClass: Error

Smalltalk createClassPreInitialize: #ZeroDivide parentClass: Error

Smalltalk createClassPreInitialize: #InvalidArgument parentClass: Error

Smalltalk createClassPreInitialize: #IndexOutofBounds parentClass: Error

Smalltalk createClassPreInitialize: #EmptyCollection parentClass: Error

Smalltalk createClassPreInitialize: #Stack

Smalltalk addInstanceVariable: #coll toClass: Stack

Smalltalk addInstanceMethod: #initialize toClass: Stack withBody: [ coll := OrderedCollection new]

Smalltalk addInstanceMethod: #isEmpty toClass: Stack withBody: [ ^ (coll size) = 0 ]

Smalltalk addInstanceMethod: #push: toClass: Stack withBody: [ :v | coll add: v ]

Smalltalk addInstanceMethod: #pop toClass: Stack withBody: [
  ^ coll removeLast
]

Smalltalk addInstanceMethod: #top toClass: Stack withBody: [
  ((coll size) = 0 )
    ifTrue:  [EmptyCollection new signal]
    ifFalse: [ ^ coll at: (coll size)]
]

Smalltalk createClassPreInitialize: #CompileError parentClass: Error

Smalltalk addInstanceMethod: #contains: toClass: ReadableString withBody:
[ :char |
  self do: [ :c |
    (char = c) ifTrue: [ ^true ]
  ].
  ^false
]

Smalltalk addInstanceMethod: #substringsForSeparators: toClass: ReadableString withBody:
[ :separators |
  | lastOcc i s arr count |
  lastOcc := 0.
  i := 1.
  s := Stack new.
  count := 0.
  self do: [ :c |
    (separators contains: c) ifTrue: [
      s push: (self substringFrom: (lastOcc + 1) to: (i - 1)).
      count := count + 1.
      lastOcc := i
    ].
    i := i + 1
  ].
  s push: (self substringFrom: (lastOcc + 1) to: (self size)).
  count := count + 1.
  arr := Array new: count.
  (1 to: count) do: [ :idx |
    arr at: (count - idx + 1) put: (s pop)
  ].
  ^arr
]

Smalltalk loadFile: 'package.st'
