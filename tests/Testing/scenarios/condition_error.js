Test.waitForCondition(() => {
    throw new Error("intentional condition failure");
}, 100);
