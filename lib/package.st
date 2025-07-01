Smalltalk createClassPreInitialize: #Package

Smalltalk addClassVariable: #packages toClass: Package

Smalltalk addClassMethod: #initialize toClass: Package withBody:
[
  packages := OrderedCollection new
]

Smalltalk addClassMethod: #packages toClass: Package withBody: [ ^ packages ]

Smalltalk addClassMethod: #getRootPackage: toClass: Package withBody:
[ :name |
  ^ (self getPackage: name withParent: nil)
]

Smalltalk addClassMethod: #getPackage:withParent: toClass: Package withBody:
[ :name :parent |
  | ret |
  packages do: [ :pkg |
    (((pkg name) = name) & ((pkg parent) = parent)) ifTrue: [ ^pkg ]
  ].
  ret := self new.
  ret name: name.
  (parent = nil) ifFalse: [
    ret parent: parent.
    ((parent childPackages) = nil) ifTrue: [ parent childPackages: (OrderedCollection new)].
    (parent childPackages) add: ret
  ].
  (self packages) add: ret.
  ^ret
]

Smalltalk addInstanceVariables: #(#name #parent #childPackages) toClass: Package

Smalltalk addInstanceMethod: #name toClass: Package withBody: [ ^ name ]

Smalltalk addInstanceMethod: #parent toClass: Package withBody: [ ^ parent ]

Smalltalk addInstanceMethod: #childPackages toClass: Package withBody: [ ^ childPackages ]

Smalltalk addInstanceMethod: #name: toClass: Package withBody: [ :v | name := v ]

Smalltalk addInstanceMethod: #parent: toClass: Package withBody: [ :v | parent := v ]

Smalltalk addInstanceMethod: #childPackages: toClass: Package withBody: [ :v | childPackages := v ]

Package initialize

Smalltalk addClassMethod: #get: toClass: Package withBody:
[ :str |
  | lastOcc i s1 s2 parent |
  i := 1.
  lastOcc := (str size) + 1.
  str do: [ :c |
    (c = $.) ifTrue: [ lastOcc := i ].
    i := i + 1
  ].
  (lastOcc = ((str size) + 1)) ifTrue: [
    ^(self getRootPackage: str)
  ]
  ifFalse: [
    s1 := str substringFrom: 1 to: (lastOcc - 1).
    s2 := str substringFrom: (lastOcc + 1) to: (str size).
    parent := self get: s1.
    ^(self getPackage: s2 withParent: parent)
  ]
]

Smalltalk addInstanceMethod: #buildStack: toClass: Package withBody:
[ :s |
  s push: name.
  (parent = nil) ifFalse: [
    parent buildStack: s
  ].
  ^s
]

Smalltalk addInstanceMethod: #getQualifiedName toClass: Package withBody:
[
  |s ret|
  s := Stack new.
  ret := ''.
  self buildStack: s.
  [s isEmpty] whileFalse: [
    ret := ret, '.' , (s pop)
  ].
  ^ret substringFrom: 2 to: (ret size)
]
