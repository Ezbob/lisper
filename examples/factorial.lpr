; recursive factorial function 
( 
  func {fac x} {
    ( 
      if ( == x 0 ) { 
        0 
      } { 
        ( * x fac ( - 1 x ) )
      } 
    )
  } 
)

; testing it out
( 
  if ( == ( fac 5 ) 120 ) {
    ( print "yep it's 120" )
  } {
    ( error "something is wrong" )
  }
)
