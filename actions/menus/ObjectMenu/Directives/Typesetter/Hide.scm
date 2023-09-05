(let ((lilytype (GetTypeAsLilypond))  (lilycontext  (GetContextAsLilypond)))
	(if lilytype
	  (StandAloneDirectiveProto (cons "HideNext" (string-append  "\\once \\override " lilycontext "." lilytype " #'stencil = ##f"  ))) #f))