; recursive factorial function 
( 
  fun {fac x} {
    (
      if ( == x 0 ) { 
        1 
      } { 
        ( * x ( fac ( - x 1 ) ) ) 
      } 
    )
  } 
)

; testing it out
( if ( == ( fac 5 ) 120 ) {
    print "it is 120 indeed"
  } {
    error "something is wrong"
  }
)
