;;;CrescDimSpanner
(if (Music?)
  (let ((tag "CrescDimSpanner")(params CrescDimSpanner::params)(text #f))
    (if (equal? params "edit")
            (set! params (RadioBoxMenu (cons (_ "Edit") 'edit)  (cons (_ "Delete") 'delete) (cons (_ "Advanced") 'advanced))))    
 
    (if (d-Directive-chord? tag)
          (cond 
            ((not params)
                (set! params 'delete))
            ((list? params)
                            (begin
                                (d-WarningDialog (_ "Sorry, this operation needs to be applied to the start of the crescendo."))
                                (set! params 'abandon)))
            ((string? params)
                        (set! text params))
            ((eq? params 'advanced)
                    (if (not (d-DirectiveTextEdit-chord tag))
                        (set! params 'delete)))
            (else
                    (set! text (d-DirectiveGet-chord-display tag)))))
     (if (not (eq? params 'abandon))   
        (begin
            (if (eq? params 'delete)
                (begin
                    (d-DirectiveDelete-chord tag)
                    (d-InfoDialog (_ "Deleted")))        
                (begin
                    (if (and (string? params) (not (equal? params "edit")))
                        (set! text params)
                        (begin
							(set! text (RadioBoxMenu 
								(cons "cresc." "cresc.")
								(cons "dim." "dim.")
								(cons "poco" "poco")
								(cons "a" "a")
								(cons "End" 'end)
								(cons "End with Dynamic" 'dynamic)
								(cons (_ "Custom text") 'custom)))
							(if (eq? text 'end)
								(begin
									(d-DirectivePut-chord-postfix tag  "\\!")
									(d-DirectivePut-chord-display tag "...!"))
								(if (eq? text 'dynamic)
									(begin
										(d-DynamicText)
										(set! text 'finished))
									(if (eq? text 'custom)
										(set! text (d-GetUserInput (_ "Crescendo Text Spanner") 
											(_ "Give text for start or continuation") "")))))))
					(if (string? text)
						(begin
							(d-DirectivePut-chord-prefix tag  (string-append
								"\\once\\set crescendoText = \\markup {\\italic { " text "}}\\once\\set crescendoSpanner = #'text "))

							(d-DirectivePut-chord-postfix tag "\\<")
							(d-DirectivePut-chord-override tag DENEMO_OVERRIDE_AFFIX)
							(d-DirectivePut-chord-display tag  text)))))
				(d-SetSaved #f)))))

