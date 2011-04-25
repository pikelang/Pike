;;; pike.el -- Font lock definitions for Pike and other LPC files.
;;; $Id$
;;; Copyright (C) 1995, 1996, 1997, 1998, 1999 Per Hedbor.
;;; This file is distributed as GPL

;;; Keywords: Pike, LPC, uLPC, µLPC, highlight

;;; To use:
;;;
;;; (require 'pike)
;;;
;;; Older Emacs versions doesn't come with a Pike-aware CC Mode (M-x
;;; c-version should report 5.23 or later), so you might have to
;;; upgrade that (see http://cc-mode.sourceforge.net/). You probably
;;; want this too in that case:
;;;
;;; (setq auto-mode-alist
;;;   (append '(("\\.pike$" . pike-mode)) auto-mode-alist))

(require 'font-lock)
(require 'custom)
(require 'cl)

(defvar font-lock-preprocessor-face	; Exists in XEmacs only.
  (or (if (facep 'font-lock-builtin-face)
	  'font-lock-builtin-face)	; The corresponding in Emacs 20.
      'font-lock-reference-face))	; The closest in Emacs 19.

(defvar font-lock-builtin-face		; Exists in Emacs 20 only.
  (or (if (facep 'font-lock-preprocessor-face)
	  'font-lock-preprocessor-face)	; The corresponding in XEmacs.
      'font-lock-reference-face))	; The closest in Emacs 19.

(defvar font-lock-constant-face		; Exists in Emacs 20 only.
  'font-lock-reference-face)		; The corresponding in other emacsen.

(defvar pike-font-lock-refdoc-face 'pike-font-lock-refdoc-face)
(defvar pike-font-lock-refdoc-init-face 'pike-font-lock-refdoc-init-face)
(defvar pike-font-lock-refdoc-init2-face 'pike-font-lock-refdoc-init2-face)
(defvar pike-font-lock-refdoc-keyword-face 'pike-font-lock-refdoc-keyword-face)
(defvar pike-font-lock-refdoc-error-face
  (if (facep 'font-lock-warning-face)
      'font-lock-warning-face		; Use the standard warning face in Emacs 20.
    'pike-font-lock-refdoc-error-face))

(defgroup pike-faces nil
  "Extra faces used by the Pike color highlighting mode."
  :group 'font-lock)

(defface pike-font-lock-refdoc-face
  '((t))
  "Face to use for normal text in Pike documentation comments.
It's overlaid over the `font-lock-comment-face'."
  :group 'pike-faces)
(defface pike-font-lock-refdoc-init-face
  '((t (:bold t)))
  "Face to use for the magic init char of Pike documentation comments.
It's overlaid over the `font-lock-comment-face'."
  :group 'pike-faces)
(defface pike-font-lock-refdoc-init2-face
  '((t))
  "Face to use for the comment starters Pike documentation comments.
It's overlaid over the `font-lock-comment-face'."
  :group 'pike-faces)
(defface pike-font-lock-refdoc-keyword-face
  '((t (:bold nil)))
  "Face to use for markup keywords Pike documentation comments.
It's overlaid over the `font-lock-builtin-face'."
  :group 'pike-faces)
(unless (facep 'font-lock-warning-face)
  (defface pike-font-lock-refdoc-error-face
    '((((class color) (background light)) (:foreground "red"))
      (((class color)) (:foreground "hotpink"))
      (((background light)) (:foreground "white" :background "black"))
      (t (:foreground "black" :background "white")))
    "Face to use for invalid markup in Pike documentation comments."
    :group 'pike-faces))

(defconst pike-font-lock-type-regexp
  (concat "\\<\\("
	  "mixed\\|float\\|int\\|program\\|string\\|"
	  "function\\|array\\|mapping\\|multiset\\|object\\|void"
	  "\\)\\>")
  "Regexp for simple types. Must have exactly one submatch.")

(defconst pike-font-lock-keyword-regexp
  (concat "\\<\\("
	  "predef\\|import\\|default\\|case\\|class\\|break\\|continue\\|"
	  "do\\|else\\|for\\|if\\|return\\|switch\\|while\\|lambda\\|"
	  "catch\\|throw\\|foreach\\|inherit\\|typedef\\|constant\\|global\\|"
	  "this_program\\|this\\)\\>")
  "Regexp for keywords. Must have exactly one submatch.")

(defconst pike-font-lock-modifier-regexp
  (concat "\\<\\(public\\|inline\\|final\\|static\\|protected\\|"
	  "local\\|optional\\|private\\|nomask\\|variant\\)\\>")
  "Regexp for modifiers. Must have exactly one submatch.")

; Problems: We really should allow all unicode characters...
(eval-and-compile
  (let ((capital-letter "A-Z\300-\326\330-\337")
	(letter "a-zA-Z\241-\377_")
	(digit  "0-9"))

    (defconst pike-font-lock-operator-identifiers
      (concat "`->=?\\|`\\+=?\\|`==\\|`\\[\\]=?\\|`()\\|`[!<>~]\\|"
	      "``?<<\\|``?>>\\|``?[%&*+/^|-]")
      "Regexp for operator identifiers. Must have zero submatches.")

    (defconst pike-font-lock-identifier-regexp
      (concat "\\(\\<[" letter "][" letter digit "]*\\>\\|"
	      pike-font-lock-operator-identifiers
	      "\\)")
      "Regexp which should match all Pike identifiers.
Must have exactly one submatch.")

    (defconst pike-font-lock-class-name-regexp
      (concat "\\<\\([" capital-letter "][" letter digit "]*\\)\\>")
      "Regexp which should match a class name.
The name is assumed to begin with a capital letter.")

    (defconst pike-font-lock-identifier-or-integer
      ;; The identifier should get submatch 1 but not the integer.
      (concat pike-font-lock-identifier-regexp "\\|-?[" digit "]+")))

  (let ((non-ws-sem-ws
	 (concat "//[^\n\r]*[\n\r]"	; Line comment.
		 "\\|"
		 ;; Block comment. We intentionally don't allow line
		 ;; breaks in them to avoid going very far and risk
		 ;; running out of regexp stack; this regexp is intended
		 ;; to handle only short comments that might be put in
		 ;; the middle of limited constructs like declarations.
		 "/\\*\\([^*\n\r]\\|\\*[^/]\\)*\\*/"
		 "\\|"
		 "\\\\[\n\r]"		; Macro continuation.
		 "\\|"
		 "@[\n\r]\\s *//[.!|]"))) ; Line continuations in autodoc comments.

    (defconst pike-font-lock-semantic-whitespace
      (concat "[ \t\n\r]*\\("		; 1
	      "\\(" non-ws-sem-ws "\\)"	; 2 3
	      "[ \t\n\r]*\\)*"))

    (defconst pike-font-lock-non-null-semantic-whitespace
      (concat "\\([ \t\n\r]\\|"		; 1
	      non-ws-sem-ws		; 2
	      "\\)+"))))

(defconst pike-font-lock-qualified-identifier
  (concat "\\([ \t\n\r]*\\(\\.[ \t\n\r]*\\)?" ; 1 2
	  pike-font-lock-identifier-regexp ; 3
	  "\\)+"))

(defconst pike-font-lock-maybe-type-end
  (concat "\\(\\.\\.\\.\\|[\]\)]\\)"	; 1
	  pike-font-lock-semantic-whitespace ; 2-4
	  "\\(\\w\\|[\(`\"'0-9]\\)" ; 5
	  "\\|"
	  "\\(\\w\\)"			; 6
 	  pike-font-lock-non-null-semantic-whitespace ; 7 8
	  "\\(\\w\\|\\<\\s_\\|[`\"'0-9]\\)")) ; 9

(defun pike-font-lock-hack-file-coding-system-perhaps ( foo )
  (interactive)
  (message "charset %s" (buffer-substring (match-beginning 2) (match-end 2)))
  (condition-case nil
      (if (and (fboundp 'set-buffer-file-coding-system)
	       (fboundp 'symbol-concat))
	  (let ((coding (buffer-substring 
			 (match-beginning 2) (match-end 2))))
	    (set-buffer-file-coding-system (symbol-concat coding))))
    (t nil)))

(defsubst pike-font-lock-beginning-of-macro ()
  "Move to the beginning of the macro containing point.
If not inside a macro, point is not moved and nil is returned.
Otherwise t is returned."
  (catch 'break
    (goto-char (save-excursion
		 (beginning-of-line)
		 (while (and (>= (- (point) 2) (point-min))
			     (eq (char-after (- (point) 2)) ?\\))
		   (forward-line -1))
		 (back-to-indentation)
		 (if (eq (following-char) ?#)
		     (point)
		   (throw 'break nil))))
    t))

(defun pike-font-lock-forward-syntactic-ws ()
  (let ((start (point)))
    ;; If forward-comment in at least XEmacs 21 is given a large
    ;; positive value, it'll loop all the way through if it hits eob.
    (while (and (forward-comment 5) (not (eobp))))
    (while (cond ((looking-at "\\\\$")
		  (forward-char)
		  t)
		 ;; Step past a macro if we were outside it to begin with.
		 ((and (eq (following-char) ?#)
		       (save-excursion
			 (beginning-of-line)
			 (and (looking-at "\\s *#")
			      (< start (point)))))
		  (while (progn
			   (end-of-line)
			   (eq (preceding-char) ?\\))
		    (forward-line 1))
		  t)
		 ;; The following handles continuation lines in autodoc comments.
		 ((looking-at "@[\n\r]\\s *//[.!|]")
		  (goto-char (match-end 0))
		  t))
      (while (and (forward-comment 5) (not (eobp)))))))

(defun pike-font-lock-backward-syntactic-ws ()
  (save-match-data
    (let ((pos (point)) (start (point)) orig)
      ;; If forward-comment in Emacs 19.34 is given a large negative
      ;; value, it'll loop all the way through if it hits bob.
      (forward-comment -20)
      (while (or (and (< (point) pos)
		      (memq (preceding-char) '(?\  ?\t ?\n ?\r ?/)))
		 (when (and (eolp) (eq (preceding-char) ?\\))
		   (backward-char)
		   t)
		 ;; Step past a macro if we were outside it to begin with.
		 (when (and (looking-at "\\s *$")
			    (< (match-end 0) start)
			    (pike-font-lock-beginning-of-macro))
		   t)
		 ;; The following handles continuation lines in autodoc comments.
		 (and (memq (preceding-char) '(?. ?! ?|))
		      (progn
			(setq orig (point))
			(beginning-of-line)
			(if (and (looking-at "\\s *//[.!|]")
				 (eq (match-end 0) orig)
				 (condition-case nil
				     (progn (backward-char 2) t)
				   (error nil))
				 (eq (following-char) ?@))
			    t
			  (goto-char orig)
			  nil))))
	(setq pos (point))
	(forward-comment -20)))))

(defvar pike-font-lock-last-type-start nil)
(defvar pike-font-lock-last-type-end nil)
(defvar pike-font-lock-real-limit nil)
(defvar pike-font-lock-more-identifiers nil)

;; The following variable records the identifiers that have been font
;; locked so far in the latest run of `pike-font-lock-find-type' and
;; the following `pike-font-lock-fontify-type's. It's used to undo all
;; the highlights in the current type if we later find chars that
;; don't belong in a type.
(defvar pike-font-lock-maybe-ids nil)

(defun pike-font-lock-find-type (limit)
  "Finds the beginning of a type."
  ;; The trick we use here is (in principle) to find two consecutive
  ;; identifiers without an operator between them. It works only if
  ;; things like keywords, comments and strings have been fontified
  ;; first.
  (let* ((start (point))
	 no-face-pos continue-pos check-macro-end cast cast-end-pos beg-pos)
    ;; We store the limit given to this function for use in the
    ;; anchored functions, since font-lock somewhat obnoxiously always
    ;; limits anchors to the same line, which we want to ignore.
    (setq pike-font-lock-real-limit limit)
    (catch 'done
      (while (re-search-forward pike-font-lock-maybe-type-end limit t)
	(if (not (memq (get-text-property (match-beginning 0) 'face)
		       '(font-lock-type-face nil)))
	    ;; Ignore a match where what we think is a type is already
	    ;; highlighted as a non-type.
	    (if (setq no-face-pos
		      (text-property-any (match-beginning 0) limit 'face nil))
		;; Jump past the highlighted region. This avoids bogus
		;; hits especially inside strings and comments.
		(goto-char no-face-pos)
	      (throw 'done nil))
	  (setq continue-pos (or (match-beginning 5) (match-beginning 9)))
	  (catch 'continue
	    (goto-char (or (match-end 1) (match-end 6)))
	    (setq check-macro-end
		  ;; If there's a newline between the end of the type and the
		  ;; following expression, the type part might end a macro.
		  ;; We'll have to check up on that later.
		  (< (save-excursion (end-of-line) (point)) continue-pos))
	    (when (setq cast (assq (preceding-char) '((?\) . ?\() (?\] . ?\[))))
	      ;; This might be a cast.
	      (setq cast-end-pos (1- (point))
		    cast (cdr cast)))
	    ;; Now at the end of something that might be a type. Back
	    ;; up to the beginning of the expression but not past a
	    ;; point that might be a preceding type or cast.
	    (condition-case nil
		(progn
		  (setq beg-pos nil)
		  (goto-char (scan-sexps (point) -1))
		  (while (and
			  (let ((prev-beg-pos beg-pos)
				(tok-beg (following-char)))
			    (setq beg-pos (point))
			    (pike-font-lock-backward-syntactic-ws)
			    (cond
			     ((memq tok-beg '(?\( ?\[))
			      ;; Cast or a subtype expression start.
			      (when (memq (char-syntax (preceding-char)) '(?w ?_))
				(goto-char (scan-sexps (point) -1))
				(if (looking-at pike-font-lock-type-regexp)
				    ;; Is a subtype expression start.
				    t
				  (unless (looking-at "return\\|case\\|break\\|continue")
				    ;; If it's a word before the paren and it's
				    ;; not one of the above it can't be a cast.
				    (if prev-beg-pos
					(setq beg-pos prev-beg-pos)
				      (throw 'continue t))
				    nil))))
			     ((memq (preceding-char) '(?| ?. ?& ?! ?~))
			      ;; Some operator valid on the top level of a type
			      ;; (yes, we're a little ahead of time here).
			      (goto-char (scan-sexps (point) -1))
			      t)
			     ((eq (preceding-char) ?#)
			      ;; Some kind of preprocessor directive.
			      (if prev-beg-pos
					(setq beg-pos prev-beg-pos)
				      (throw 'continue t))
			      nil)))
			  (>= (point) start))
		    (setq cast nil)))
	      ;; Should only get here if the scan-sexps above fails.
	      (error nil))
	    (when beg-pos
	      (goto-char beg-pos)
	      (when (and check-macro-end
			 ;; End of the type and beg of the expression are on
			 ;; different lines...
			 (save-excursion
			   (goto-char continue-pos)
			   (beginning-of-line)
			   ;; bob can't happen here.
			   (not (eq (char-after (- (point) 2)) ?\\)))
			 ;; ...and the expression is not part of a macro...
			 (pike-font-lock-beginning-of-macro))
		;; ...but the type is. They therefore don't correspond to each
		;; other and we should ignore it. Set continue-pos to continue
		;; searching after the macro. We also set start so we don't go
		;; back into the macro a second time.
		(goto-char beg-pos)
		(end-of-line)
		(while (and (not (eobp))
			    (eq (preceding-char) ?\\))
		  (forward-char)
		  (end-of-line))
		(setq start (point)
		      continue-pos (point))
		(throw 'continue t))
	      (setq pike-font-lock-last-type-start (max start (point))
		    pike-font-lock-more-identifiers nil
		    pike-font-lock-maybe-ids nil)
	      (when (eq (following-char) cast)
		;; Jumped over exactly one sexp surrounded with ( ) or [ ],
		;; so it's a cast.
		(forward-char)
		;; Make sure that pike-font-lock-find-following-identifier
		;; doesn't highlight any following identifier.
		(setq pike-font-lock-last-type-end cast-end-pos)
		(throw 'done t))
	      (setq pike-font-lock-last-type-end continue-pos)
	      (throw 'done t)))
	  (goto-char continue-pos)
	  )))))

(defun pike-font-lock-fontify-type (limit)
  "Finds the next identifier/keyword in a type.
Used after `pike-font-lock-find-type' or `pike-font-lock-fontify-type'
have matched."
  (when (< (point) pike-font-lock-last-type-end)
    (catch 'done
      (while t
	(while (prog2
		   (pike-font-lock-forward-syntactic-ws)
		   (> (skip-chars-forward ":().|,&!~") 0)
		 (when (>= (point) pike-font-lock-last-type-end)
		   (goto-char pike-font-lock-last-type-end)
		   (throw 'done nil))))
	(if (and (not (eq (get-text-property (point) 'face) 'font-lock-type-face))
		 (looking-at pike-font-lock-identifier-or-integer))
	    (if (< (point) pike-font-lock-last-type-start)
		(goto-char (match-end 0))
	      (when (match-beginning 1)
		(setq pike-font-lock-maybe-ids
		      (cons (cons (match-beginning 1) (match-end 1))
			    pike-font-lock-maybe-ids)))
	      (goto-char (match-end 0))
	      (throw 'done t))
	  ;; Turned out to be too expression-like to be highlighted as
	  ;; a type; undo the highlights. By checking for words
	  ;; already highlighted as types above we catch the case when
	  ;; an expression that contains types is about to be
	  ;; highlighted as a type itself.
	  (while pike-font-lock-maybe-ids
	    (let ((range (car pike-font-lock-maybe-ids)))
	      (when (eq (get-text-property (car range) 'face) 'font-lock-type-face)
		(put-text-property (car range) (cdr range) 'face nil))
	      (setq pike-font-lock-maybe-ids (cdr pike-font-lock-maybe-ids))))
	  (goto-char pike-font-lock-last-type-end)
	  (throw 'done nil))))))

(defun pike-font-lock-find-following-identifier (limit)
  "Finds the following identifier after a type.
Used after `pike-font-lock-find-type', `pike-font-lock-fontify-type'
or `pike-font-lock-find-following-identifier' have matched. Should the
variable name be followed by a comma after an optional value, we
reposition the cursor to fontify more identifiers."
  (if (and pike-font-lock-maybe-ids
	   (< (point) pike-font-lock-real-limit)
	   (looking-at pike-font-lock-identifier-regexp))
      (let ((match (match-data))
	    (start (point))
	    (value-start (match-end 1))
	    (more pike-font-lock-more-identifiers)
	    chr)
	(goto-char value-start)
	(save-restriction
	  (narrow-to-region (point-min) pike-font-lock-real-limit)
	  (pike-font-lock-forward-syntactic-ws)
	  (setq chr (following-char)
		pike-font-lock-more-identifiers nil)
	  (prog1
	      (cond
	       ((and (eq chr ?\() (not more))
		;; It's a function identifier. Make the first submatch
		;; second to get the right face.
		(setcdr (cdr match) (cons nil (cons nil (cdr (cdr match)))))
		t)
	       ((eq chr ?,)
		;; It's a variable identifier in a list containing more
		;; identifiers.
		(forward-char)
		(pike-font-lock-forward-syntactic-ws)
		;; Signal that we've gone on looking at an identifier that
		;; isn't first in a list. It can't be a function then.
		(setq pike-font-lock-more-identifiers t)
		t)
	       ((memq chr '(?\; ?\) ?\]))
		;; It's a variable identifier at the end of a list.
		t)
	       ((eq chr ?=)
		;; It's a variable identifier with a value assignment.
		;; Move over it to the next comma, if any.
		(condition-case nil
		    (progn
		      ;; Note: Both `scan-sexps' and the second goto-char can
		      ;; generate an error which is caught by the
		      ;; `condition-case' expression.
		      (while (progn
			       (pike-font-lock-forward-syntactic-ws)
			       (not (looking-at "\\(\\(,\\)\\|;\\|$\\)")))
			(goto-char (or (scan-sexps (point) 1) (point-max))))
		      (goto-char (match-end 2)) ; non-nil
		      (pike-font-lock-forward-syntactic-ws))
		  (error
		   (goto-char value-start)))
		t)
	       ((= (point) pike-font-lock-real-limit)
		t)
	       (t
		(if more (goto-char start))
		nil))
	    (set-match-data match))))
    ;; If it was a cast we must skip forward a bit to not recognize it again.
    (skip-chars-forward ")]" pike-font-lock-real-limit)
    nil))

(defun pike-font-lock-find-label (limit)
  (catch 'found
    (while (re-search-forward (concat pike-font-lock-identifier-regexp ":[^:]") limit t)
      (unless
	  ;; Ignore hits inside highlighted stuff.
	  (get-text-property (match-beginning 1) 'face)
	(save-excursion
	  (goto-char (match-beginning 1))
	  (pike-font-lock-backward-syntactic-ws)
	  (if (or
	       ;; Check for a char that precedes a statement.
	       (memq (preceding-char) '(?\} ?\{ ?\) ?\;))
	       ;; Check for a keyword that precedes a statement.
	       (and (condition-case nil
			(progn (backward-sexp) t)
		      (error nil))
		    (save-match-data
		      (looking-at "\\(else\\|do\\)\\>"))))
	      (throw 'found t)))))))

(defconst pike-font-lock-autodoc-comment-1
  ;; This part matches a line oriented keyword with
  ;; continuation lines.
  (concat "\\(//\\)\\([.!|]\\)"		; 1 2
	  "\\("				; 3
	  "\\s *@\\([A-Za-z_]+\\)\\(\\s \\|$\\)" ; 4 5
	  "\\([^@\n\r]\\|@\\(.\\|\\([\n\r]\\s *//[.!|]\\)\\)\\)*" ; 6-8
	  "@?\\)"
	  "\\([\n\r]\\|\\'\\)"))	; 9

(defconst pike-font-lock-autodoc-comment-2
  ;; This part matches a block with no line oriented keywords.
  ;; (Negations become messy easily, especially while ensuring
  ;; that no bad backtracking occurs...)
  ;;
  ;; Note: This regexp used to handle continued lines beginning with a
  ;; line keyword correctly (i.e. marking it as an error), but the
  ;; regexp engine in Emacs 20 has become too brittle, poor thing..
  (concat "\\(//\\)\\([.!|]\\)"		; 1 2
	  "\\("				; 10
	  "\\s *"
	  (concat "\\("			; 11
		  (concat "\\("		; 12
			  "[^@ \t\n\r]\\|"
			  (concat "@\\(" ; 13
				  "[^A-Za-z_\n\r]\\|"
				  "[A-Za-z_]+[^A-Za-z_ \t\n\r]"
				  "\\)")
			  "\\)"
			  "[^\n\r]*")
		  "\\)?")
	  (concat "\\("			; 14
		  "[\n\r]\\s *//[.!|]"
		  "\\s *"
		  (concat "\\("		; 15
			  (concat "\\("	; 16
				  "[^@ \t\n\r]\\|"
				  (concat "@\\(" ; 17
					  "[^A-Za-z_\n\r]\\|"
					  "[A-Za-z_]+[^A-Za-z_ \t\n\r]"
					  "\\)")
				  "\\)"
				  "[^\n\r]*")
			  "\\)?")
		  "\\)*")
	  "@?\\)"
	  "\\([\n\r]\\|\\'\\)"))	; 18

(defun pike-font-lock-find-autodoc (limit)
  (catch 'found
    (while (re-search-forward "^\\([^/\n\r]\\|/[^/\n\r]\\)*\\(//\\)[.!|]" limit t)
      (goto-char (match-beginning 2))
      (when (save-restriction
	      (narrow-to-region (point) limit)
	      (cond ((looking-at pike-font-lock-autodoc-comment-1)
		     t)
		    ((looking-at pike-font-lock-autodoc-comment-2)
		     (let ((match (match-data)))
		       (setcdr (nthcdr 5 match)
			       (nconc (make-list 14 nil)
				      (nthcdr 6 match)))
		       (set-match-data match))
		     t)))
	(goto-char (setq pike-font-lock-real-limit (match-end 0)))
	(throw 'found t)))
    nil))

;; For use inside (function ...) since we can't use , there to do the
;; constant concatenation. (Works only after byte compilation.)
(defmacro pike-font-lock-concat-const (&rest consts)
  (apply 'concat (mapcar 'eval consts)))

(defconst pike-font-lock-some
  `(;; Keywords:
    (,pike-font-lock-keyword-regexp
     1 font-lock-keyword-face)
	 
    ;; Modifiers:
    (,pike-font-lock-modifier-regexp
     1 font-lock-keyword-face)

    ;; Class names:
    (,(concat "\\(\\<class\\>\\)\\s *"
	      pike-font-lock-identifier-regexp)
     (1 font-lock-keyword-face)
     (2 font-lock-function-name-face))

    ;; Labels in statements:
    (,(concat "\\<\\(break\\|continue\\)\\s *"
	      pike-font-lock-identifier-regexp)
     2 ,font-lock-constant-face)))

(defconst pike-font-lock-more
  (append `(("^#!.*$" 0 font-lock-comment-face)

	    ("^[ \t]*#[ \t]*error\\(.*\\)$"
	     (1 font-lock-string-face))

	    ;; #charset char-set-name
	    ;; Defines the file charset.
	    ("^[ \t]*\\(#[ \t]*charset\\)[ \t]*\\(.*\\)$"
	     (2 font-lock-keyword-face)
	     (pike-font-lock-hack-file-coding-system-perhaps))

	    ;; Fontify filenames in #include <...> as strings.
	    ("^[ \t]*#[ \t]*include[ \t]+\\(<[^>\"\n]+>\\)"
	     1 font-lock-string-face)

	    ;; #define
	    (,(concat "^\\s *#\\s *define\\s +"
		      "\\("		; 1
		      (concat pike-font-lock-identifier-regexp "\(\\|" ; 2
			      pike-font-lock-identifier-regexp "\\(\\s \\|$\\)") ; 3 4
		      "\\)")
	     (2 font-lock-function-name-face nil t)
	     (3 font-lock-variable-name-face nil t))

	    ;; Fontify symbol names in #if ...defined 
	    ;; etc preprocessor directives.
	    ("^[ \t]*#[ \t]*\\(el\\)?if\\>"
	     (,(concat "\\<\\(defined\\|efun\\|constant\\)\\>[ \t]*" ; 1
		       "(?" pike-font-lock-identifier-regexp "?") ; 2
	      nil nil
	      (1 ,font-lock-preprocessor-face)
	      (2 font-lock-variable-name-face nil t)))

	    (,(concat "^[ \t]*#[ \t]*ifn?def[ \t]+"
		      pike-font-lock-identifier-regexp)
	     (1 font-lock-variable-name-face))

	    ("^[ \t]*\\(#[ \t]*[a-z]+\\)\\>"
	     (1 ,font-lock-preprocessor-face)
	     (2 font-lock-variable-name-face nil t)))

	  pike-font-lock-some

	  `(;; Case labels with a qualified identifier.
	    (,(concat "\\<case\\>\\s *\\("
		      pike-font-lock-qualified-identifier
		      "\\):")
	     1 ,font-lock-constant-face)

	    ;; Scope references.
	    (,(function
	       (lambda (limit)
		 (when (re-search-forward
			(pike-font-lock-concat-const
			 pike-font-lock-identifier-regexp ; 1
			 "[ \t\n\r]*\\(\\.\\|::\\)" ; 2
			 "[ \t\n\r]*\\(\\w\\|`\\)") ; 3
			limit t)
		   ;; Must back up the last bit since it can be the next
		   ;; identifier to match.
		   (goto-char (match-beginning 3)))))
	     1 ,font-lock-constant-face)

	    ;; Inherits.
	    (,(concat "\\<inherit\\s +"
		      "\\(" pike-font-lock-qualified-identifier "\\)" ; 1-4
		      "[ \t\n\r]*\\(:"	; 5
		      pike-font-lock-semantic-whitespace ; 6-8
		      pike-font-lock-identifier-regexp ; 9
		      "\\)?")
	     (9 ,font-lock-constant-face nil t)
	     (,(function
		(lambda (limit)
		  (when (looking-at (pike-font-lock-concat-const
				     "\\.?[ \t\n\r]*"
				     pike-font-lock-identifier-regexp
				     "[ \t\n\r]*"))
		    (goto-char (match-end 0)))))
	      (progn (goto-char (match-beginning 1)) nil)
	      (goto-char (match-end 0))
	      (1 font-lock-type-face))))))

(defconst pike-font-lock-more-lastly
  `(;; Constants. This is after type font locking to get typed
    ;; constants correct (if and when they get valid).
    (,(concat "\\<constant[ \t\n\r]+"
	      pike-font-lock-identifier-regexp)
     1 font-lock-variable-name-face)

    ;; Simple types that might not have been catched by the
    ;; earlier rules.
    (,pike-font-lock-type-regexp
     1 font-lock-type-face)

    ;; Labels. Do this after the type font locking to avoid
    ;; highlighting "int" in "mapping(int:string)" as a label.
    (pike-font-lock-find-label
     1 ,font-lock-constant-face)))

(defconst pike-font-lock-keywords-1
  (append pike-font-lock-some

	  `(;; Case labels followed by anything.
	    (,(concat "\\<case\\>\\s *\\(.*\\):")
	     1 font-lock-reference-face)))
  "For consideration as a value of `pike-font-lock-keywords'.
This does fairly subdued highlighting.")

(defconst pike-font-lock-keywords-2
  (let ((decl-start (concat "\\("	; 1
			    pike-font-lock-type-regexp ; 2
			    "\\|"
			    "^\\s *"
			    (concat "\\(" ; 3
				    pike-font-lock-class-name-regexp ; 4
				    (concat "\\(\\." ; 5
					    pike-font-lock-identifier-regexp ; 6
					    "\\)*")
				    "\\)")
			    "\\)"
			    "[ \t\n\r]*")))
    (append pike-font-lock-more

	    `(;; Function declarations.
	      (,(concat decl-start
			pike-font-lock-identifier-regexp ; 7
			"[ \t\n\r]*(")
	       (3 font-lock-type-face nil t)
	       (7 font-lock-function-name-face))

	      ;; Variable declarations.
	      (,(concat decl-start
			pike-font-lock-identifier-regexp) ; 7
	       (3 font-lock-type-face nil t)
	       (7 font-lock-variable-name-face)))

	    pike-font-lock-more-lastly))
  "For consideration as a value of `pike-font-lock-keywords'.
This adds as much highlighting as possible without making it too
expensive to calculate. Among other things that means that only simple
types are recognized.")

(defconst pike-font-lock-keywords-3
  (append `(;; Autodoc comments.
	    (pike-font-lock-find-autodoc
	     (1 pike-font-lock-refdoc-init2-face prepend)
	     (2 pike-font-lock-refdoc-init-face prepend)
	     (3 ,font-lock-builtin-face t t)
	     (3 pike-font-lock-refdoc-keyword-face prepend t)
	     (10 pike-font-lock-refdoc-face prepend t)

	     ;; A Pike declaration inside the comment. Reset the
	     ;; comment highlight so that we can redo it as code.
	     (,(function
		(lambda (limit)
		  (when (and (< (point) pike-font-lock-real-limit)
			     (looking-at "decl\\|member\\|index\\|elem"))
		    (put-text-property (match-end 0) pike-font-lock-real-limit
				       'face nil)
		    nil)))
	      (progn (goto-char (or (match-beginning 4) pike-font-lock-real-limit))
		     nil)
	      nil)

	     ;; Must (re)handle the string literals ourselves inside
	     ;; the declaration.
	     (,(function
		(lambda (limit)
		  (re-search-forward "\"[^\"]*\"\\|'[^']*'"
				     pike-font-lock-real-limit t)))
	      (progn (goto-char (or (match-end 5) pike-font-lock-real-limit))
		     nil)
	      nil
	      (0 font-lock-string-face))

	     ;; Special case when the declaration is a #define.
	     (,(function
		(lambda (limit)
		  (prog1
		      (and (< (point) pike-font-lock-real-limit)
			   (looking-at (pike-font-lock-concat-const
					pike-font-lock-semantic-whitespace ; 1-3
					"\\(#\\s *define\\)\\s +" ; 4
					"\\(" ; 5
					(concat "\\(\\w+\\)\(\\|" ; 6
						"\\(\\w+\\)\\(\\s \\|$\\)") ; 7 8
					"\\)")))
		    (goto-char pike-font-lock-real-limit))))
	      (progn (goto-char (or (match-end 5) pike-font-lock-real-limit))
		     nil)
	      nil
	      (4 ,font-lock-builtin-face)
	      (6 font-lock-function-name-face nil t)
	      (7 font-lock-variable-name-face nil t))

	     (,(function
		(lambda (limit)
		  (re-search-forward
		   (pike-font-lock-concat-const
		    "\\(@\\(\\w+{\\|\\[\\([^\]@]\\|@@\\)*\\]\\|[@}]\\|$\\)\\)\\|"
		    "\\(@\\(\\w+\\|\\W\\)\\)")
		   pike-font-lock-real-limit t)))
	      ;; In-text markup and markup errors.
	      (progn (goto-char (or (match-end 5) (match-end 2)))
		     nil)
	      nil
	      (1 ,font-lock-builtin-face t t)
	      (1 pike-font-lock-refdoc-keyword-face prepend t)
	      (4 ,pike-font-lock-refdoc-error-face t t))

	     ;; Markup line continuations. Uses match 8 or 15 in
	     ;; pike-font-lock-autodoc-comment to decide whether
	     ;; there's anything to do at all.
	     (,(function
		(lambda (limit)
		  (re-search-forward "[\n\r]\\(\\s *\\(//\\)\\([.!|]\\)\\)"
				     pike-font-lock-real-limit t)))
	      (progn (goto-char (if (or (match-end 8) (match-end 15))
				    (match-end 2)
				  pike-font-lock-real-limit))
		     nil)
	      (goto-char pike-font-lock-real-limit)
	      (1 font-lock-comment-face t)
	      (2 pike-font-lock-refdoc-init2-face prepend)
	      (3 pike-font-lock-refdoc-init-face prepend))))

	  pike-font-lock-more

	  `(;; Catches declarations and casts except in the strangest cases.
	    (pike-font-lock-find-type
	     (pike-font-lock-fontify-type
	      nil nil
	      (1 font-lock-type-face nil t))
	     (pike-font-lock-find-following-identifier
	      nil nil
	      (1 font-lock-variable-name-face nil t)
	      (2 font-lock-function-name-face nil t))))

	  pike-font-lock-more-lastly)
  "For consideration as a value of `pike-font-lock-keywords'.
This adds highlighting that can be quite computationally intensive but
provides almost entirely correct highlighting of e.g. types.")

(defvar pike-font-lock-keywords pike-font-lock-keywords-1
  "Additional expressions to highlight in Pike mode.")

;; XEmacs way.
(put 'pike-mode 'font-lock-defaults 
     '((pike-font-lock-keywords
	pike-font-lock-keywords-1
	pike-font-lock-keywords-2
	pike-font-lock-keywords-3)
       nil nil ((?_ . "w")) beginning-of-defun
       (font-lock-mark-block-function . mark-defun)))

;; GNU Emacs way.
(if (and (boundp 'font-lock-defaults-alist)
	 (not (assq 'pike-mode font-lock-defaults-alist)))
    (setq font-lock-defaults-alist
	  (cons
	   (cons 'pike-mode
		 '((pike-font-lock-keywords
		    pike-font-lock-keywords-1
		    pike-font-lock-keywords-2
		    pike-font-lock-keywords-3)
		   nil nil ((?_ . "w")) beginning-of-defun
		   (font-lock-mark-block-function . mark-defun)))
	   font-lock-defaults-alist)))

;; Autoload spec for older emacsen that doesn't come with a Pike aware
;; CC Mode. Doesn't do any harm in later emacsen.
(autoload 'pike-mode "cc-mode" "Major mode for editing Pike code.")

(provide 'pike)
