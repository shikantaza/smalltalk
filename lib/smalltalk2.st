Smalltalk assignClass: Object toPackage: 'core'

Smalltalk assignClass: Smalltalk toPackage: 'core'

Smalltalk assignClass: Nil toPackage: 'core'

Smalltalk assignClass: Transcript toPackage: 'core'

Smalltalk assignClass: Integer toPackage: 'core'

Smalltalk assignClass: Float toPackage: 'core'

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

Smalltalk assignClass:  Symbol toPackage: 'core.util.collections'

Smalltalk createClass: #ExceptionSet
  parentClass: Object
  instanceVars: #(#exceptions)
  classVars: #()
  inPackage: 'core'

Smalltalk addInstanceMethod: #initialize toClass: ExceptionSet withBody:
[
  exceptions := OrderedCollection new
]

Smalltalk addInstanceMethod: #handles: toClass: ExceptionSet withBody:
[
  :excp |
  exceptions do: [ :ex |
    (ex handles: excp) ifTrue: [ ^ true]
  ].
  ^ false
]

Smalltalk addInstanceMethod: #, toClass: ExceptionSet withBody:
[
  :excp |
  exceptions add: excp
]

Smalltalk addClassMethod: #handles: toClass: Exception withBody:
[
  :excp |
  ^ excp isKindOf: self
]

Smalltalk addClassMethod: #, toClass: Exception withBody:
[
  :excp |
  | excpSet |
  excpSet := ExceptionSet new.
  excpSet , self; , excp.
  ^ excpSet
]
