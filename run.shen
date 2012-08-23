(package run- [*stoutput* shen-*hush* shen-hushed main]

(synonyms main-fn ((list string) --> boolean))

(set *null* "/dev/null")
(set *error* "shen_err.log")

(define call-with-saving-stoutput
  F -> (let stdout (value *stoutput*)
         (trap-error (let Ret (thaw F)
                          - (set *stoutput* stdout)
                       Ret)
                     (/. E (do (set *stoutput* stdout)
                               (error (error-to-string E)))))))

(define call-with-null-output
  F -> (call-with-saving-stoutput
         (freeze (let - (set *stoutput* (open file (value *null*) out))
                      Ret (thaw F)
                      - (close (value *stoutput*))
                   Ret))))

(define call-with-error-file
  F -> (trap-error (thaw F)
                   (/. E (let F (open file (value *error*) out)
                              - (pr (error-to-string E) F)
                              - (close F)
                           _))))

(define script
  {(list string) --> (list string) --> main-fn --> boolean}
  Args File Main -> (call-with-error-file
                      (let - (call-with-null-output (freeze (load File)))
                           - (set shen-*hush* shen-hushed)
                           - (output "c#34;c#34;c#34;~%")
                        (freeze (if (empty? Main)
                                    (main Args)
                                    (Main Args))))))
)