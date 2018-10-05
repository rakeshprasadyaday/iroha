PREPARE addAssetQuantityWithOutValidation3 (text, text, int, text) AS
          WITH has_account AS (SELECT account_id FROM account
                               WHERE account_id = $1 LIMIT 1),
               has_asset AS (SELECT asset_id FROM asset
                             WHERE asset_id = $2 AND
                             precision >= $3 LIMIT 1),

               amount AS (SELECT amount FROM account_has_asset
                          WHERE asset_id = $2 AND
                          account_id = $1 LIMIT 1),
               new_value AS (SELECT $4::decimal +
                              (SELECT
                                  CASE WHEN EXISTS
                                      (SELECT amount FROM amount LIMIT 1) THEN
                                      (SELECT amount FROM amount LIMIT 1)
                                  ELSE 0::decimal
                              END) AS value
                          ),
               inserted AS
               (
                  INSERT INTO account_has_asset(account_id, asset_id, amount)
                  (
                      SELECT $1, $2, value FROM new_value
                      WHERE EXISTS (SELECT * FROM has_account LIMIT 1) AND
                        EXISTS (SELECT * FROM has_asset LIMIT 1) AND
                        EXISTS (SELECT value FROM new_value
                                WHERE value < 2::decimal ^ (256 - $3)
                                LIMIT 1)

                  )
                  ON CONFLICT (account_id, asset_id) DO UPDATE
                  SET amount = EXCLUDED.amount
                  RETURNING (1)
               )
          SELECT CASE
              WHEN EXISTS (SELECT * FROM inserted LIMIT 1) THEN 0
              WHEN NOT EXISTS (SELECT * FROM has_account LIMIT 1) THEN 2
              WHEN NOT EXISTS (SELECT * FROM has_asset LIMIT 1) THEN 3
              WHEN NOT EXISTS (SELECT value FROM new_value
                               WHERE value < 2::decimal ^ (256 - $3)
                               LIMIT 1) THEN 4
              ELSE 5
          END AS result;
