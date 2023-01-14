;;;PageNumbersWithInstrument
(let ((tag "PageNumbersWithInstrument"))
(if (d-Directive-paper? tag)
    (begin
        (d-DirectiveDelete-paper tag)
        (d-InfoDialog (_ "Default Page Numbering Restored")))
(d-DirectivePut-paper-postfix tag
"
   oddHeaderMarkup = \\markup
   \\fill-line {
     \"\"
     \\unless \\on-first-page-of-part \\fromproperty #'header:instrumentation
     \\if \\should-print-page-number \\number \\fromproperty 
#'page:page-number-string
   }

   %% evenHeaderMarkup would inherit the value of
   %% oddHeaderMarkup if it were not defined here
    evenHeaderMarkup = \\markup
   \\fill-line {
     \\if \\should-print-page-number \\number \\fromproperty 
#'page:page-number-string
     \\unless \\on-first-page-of-part \\fromproperty #'header:instrumentation
     \"\"
   }
"))
(d-SetSaved #f))
