<div class="row vertical-offset-100">
    <div class="col-md-4 col-md-offset-4">
        <div class="panel panel-default">
            <div class="panel-heading text-center">
                <h3 class="panel-title">BlueTrack Login</h3>
            </div>
            <div class="panel-body">
                <form accept-charset="UTF-8" role="form" method="post" action="">
                    <fieldset>
                        <div class="form-group<?php echo($form_errors->has('username') ? ' has-error' : '') ?>">
                            <input class="form-control" placeholder="Username" name="username" type="text" value="<?php PrintOldFormValue('username'); ?>" required autofocus>
                            <?php
                            if($form_errors->has('username')) {
                                ?>
                                <span class="help-block">
                                            <strong><?php echo $form_errors->getErrorMsg('username'); ?></strong>
                                        </span>
                                <?php
                            }
                            ?></div>
                        <div class="form-group<?php echo($form_errors->has('password') ? ' has-error' : '') ?>">
                            <input class="form-control" placeholder="Password" name="password" type="password" value="" required>
                            <?php
                            if($form_errors->has('password')) {
                                ?>
                                <span class="help-block">
                                            <strong><?php echo $form_errors->getErrorMsg('password'); ?></strong>
                                        </span>
                                <?php
                            }
                            ?></div>
                        <input class="btn btn-lg btn-success btn-block" type="submit" value="Login">
                    </fieldset>
                </form>
            </div>
        </div>
    </div>
</div>