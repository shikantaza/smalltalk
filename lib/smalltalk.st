Smalltalk addClassMethod: #createClass:inPackage: toClass: Smalltalk withBody:
[ :className :package |
  | ret |
  ret := self createClassPreInitialize: className.
  self assignClass: ret toPackage: package.
  ^ ret
]

Smalltalk addClassMethod: #createClass:parentClass:inPackage: toClass: Smalltalk withBody:
[ :className :parentClass :package |
  | ret |
  ret := self createClassPreInitialize: className parentClass: parentClass.
  self assignClass: ret toPackage: package.
  ^ ret
]

Smalltalk addClassMethod: #createClass: toClass: Smalltalk withBody:
[ :className |
  | ret |
  ret := self createClassPreInitialize: className.
  self assignClass: ret toPackage: 'user'.
  ^ ret
]

Smalltalk addClassMethod: #createClass:parentClass: toClass: Smalltalk withBody:
[ :className :parentClass |
  | ret |
  ret := self createClassPreInitialize: className parentClass: parentClass.
  self assignClass: ret toPackage: 'user'.
  ^ ret
]

Smalltalk addClassMethod: #createClass:parentClass:instanceVars:classVars:inPackage: toClass: Smalltalk withBody:
[ :className :parentClass :instVars :classVars :package |
  | ret |
  ret := self createClass: className parentClass: parentClass inPackage: package.
  self addInstanceVariables: instVars toClass: ret.
  self addClassVariables: classVars toClass: ret.
  ^ret
]

Smalltalk addClassMethod: #createClass:parentClass:instanceVars:classVars:inPackage:aliases: toClass: Smalltalk withBody:
[ :className :parentClass :instVars :classVars :package :aliases |
  |ret|
  [
    ret := self createClass: className parentClass: parentClass inPackage: package.
    self addInstanceVariables: instVars toClass: ret.
    self addClassVariables: classVars toClass: ret.
    aliases do: [ :entry |
      self createAlias: (entry at: 1) for: (entry at: 2) fromPackage: (entry at: 3) inClass: ret
    ].
    ^ret
  ] on: Exception do: [ :ex |
    ret = nil ifFalse: [ self deleteClass: ret. ex signal ]
  ]
]

Smalltalk addClassMethod: #createClass:instanceVars:classVars:inPackage: toClass: Smalltalk withBody:
[ :className :instVars :classVars :package |
  ^(self createClass: className parentClass: Object instanceVars: instVars classVars: classVars inPackage: package)
]

Smalltalk addClassMethod: #addInstanceVariables:toClass: toClass: Smalltalk  withBody: [
  :lst :cls |
  lst do: [ :v | self addInstanceVariable: v toClass: cls ]
]

Smalltalk addClassMethod: #addClassVariables:toClass: toClass: Smalltalk  withBody: [
  :lst :cls |
  lst do: [ :v | self addClassVariable: v toClass: cls ]
]

Smalltalk createClass: #Error parentClass: Exception inPackage: 'core'

Smalltalk createClass: #MessageNotUnderstood parentClass: Error inPackage: 'core'

Smalltalk createClass: #ZeroDivide parentClass: Error inPackage: 'core'

Smalltalk createClass: #InvalidArgument parentClass: Error inPackage: 'core'

Smalltalk createClass: #IndexOutofBounds parentClass: Error inPackage: 'core'

Smalltalk createClass: #EmptyCollection parentClass: Error inPackage: 'core'

Smalltalk createClass: #Stack inPackage: 'core.util.collections'

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

Smalltalk createClass: #CompileError parentClass: Error inPackage: 'core'

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

Smalltalk loadFile: 'smalltalk2.st'
