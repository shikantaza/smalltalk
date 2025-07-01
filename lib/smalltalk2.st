Smalltalk assignClass: Object toPackage: 'core'

Smalltalk assignClass: Smalltalk toPackage: 'core'

Smalltalk assignClass: Nil toPackage: 'core'

Smalltalk assignClass: Transcript toPackage: 'core'

Smalltalk assignClass: Integer toPackage: 'core'

Smalltalk assignClass: NiladicBlock toPackage: 'core'

Smalltalk assignClass: MonadicBlock toPackage: 'core'

Smalltalk assignClass: DyadicValuable toPackage: 'core'

Smalltalk assignClass: Boolean toPackage: 'core'

Smalltalk assignClass: Exception toPackage: 'core'

Smalltalk assignClass: Array toPackage: 'core.util.collections'

Smalltalk assignClass: OrderedCollection toPackage: 'core.util.collections'

Smalltalk assignClass: Compiler toPackage: 'core'

Smalltalk assignClass: ReadableString toPackage: 'core.util.collections'

Smalltalk assignClass: Character toPackage: 'core'

Smalltalk assignClass:  Error toPackage: 'core'

Smalltalk assignClass:  MessageNotUnderstood toPackage: 'core'

Smalltalk assignClass:  ZeroDivide toPackage: 'core'

Smalltalk assignClass:  InvalidArgument toPackage: 'core'

Smalltalk assignClass:  IndexOutofBounds toPackage: 'core'

Smalltalk assignClass:  EmptyCollection toPackage: 'core'

Smalltalk assignClass: Stack toPackage: 'core.util.collections'

Smalltalk assignClass:  CompileError toPackage: 'core'

Smalltalk assignClass:  Package toPackage: 'core'

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

Smalltalk addClassMethod: #createClass:instanceVars:classVars:inPackage: toClass: Smalltalk withBody:
[ :className :instVars :classVars :package |
  ^(self createClass: className parentClass: Object instanceVars: instVars classVars: classVars inPackage: package)
]

Package get: 'user'
