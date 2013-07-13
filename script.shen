(package shen-run [*stoutput* shen.*hush* shen.hushed main out in]

(set *null* "/dev/null")
(set args [])

(define call-with-saving-stoutput
  F -> (let stdout (value *stoutput*)
         (trap-error (let Ret (thaw F)
                          - (set *stoutput* stdout)
                       Ret)
                     (/. E (do (set *stoutput* stdout)
                               (error (error-to-string E)))))))

(define call-with-null-output
  F -> (call-with-saving-stoutput
         (freeze (let - (set *stoutput* (open (value *null*) out))
                      Ret (thaw F)
                      - (close (value *stoutput*))
                   Ret))))

(define add-arg
  X -> (set args [X | (value args)]))

(define execute
  Args File Main -> (call-with-err
                      (freeze
                        (let - (call-with-null-output (freeze (load File)))
                             - (set shen.*hush* shen.hushed)
                             - (output "~%c#34;c#34;c#34;c#34;c#34;")
                             Ret (if (empty? Main)
                                     (main Args)
                                     (Main Args))
                            - (if Ret
                                  (exit)
                                  (error "c#10;"))
                          Ret)))))
